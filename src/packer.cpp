#include "packer.h"
#include "bleeding.h"
#include "png/png.h"
#include "rbp/MaxRects.h"
#include "json/json.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <iterator>
#include <cmath>
#include <cstdio>
#include <stdint.h>
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>

#define countof(x) (sizeof(x) / sizeof(x[0]))

#ifdef _WIN32
#define mkdir(a,b) mkdir(a)
#endif

namespace pkr
{
	struct Sprite
	{
		const char *filename;
		int x;
		int y;
		int width;
		int height;
		bool rotated;
	};

	struct Result
	{
		int width;
		int height;
		std::vector<Sprite> sprites;
	};

	struct Packer
	{
		Params params;

		std::vector<char*> filenames;
		std::vector<char> filenamesbuf;

		std::vector<Sprite> input_sprites;
		std::vector<rbp::RectSize> input_rects;

		Json::Value metadata;

		Packer(const Params &params) : params(params) {}

		int pack_mode(const char *mode)
		{
			static const char *modes[] = {
				"auto",
				"short-side",
				"long-side",
				"best-area",
				"bottom-left",
				"contact-point"
			};

			for (size_t i = 0; i < countof(modes); i++)
			{
				if (strcmp(mode, modes[i]) == 0)
					return i;
			}

			return -1;
		}

		bool validate_params()
		{
			if (params.output == 0)
			{
				fprintf(stderr, "The argument --output is required.\n");
				return false;
			}

			if (params.padding < 0)
			{
				fprintf(stderr, "Invalid padding.\n");
				return false;
			}

			if ((params.width > 0 || params.height > 0) &&
				(params.width <= 0 || params.height <= 0))
			{
				fprintf(stderr, "Invalid size.\n");
				return false;
			}

			if ((params.max_width > 0 || params.max_height > 0) &&
				(params.max_width <= 0 || params.max_height <= 0))
			{
				fprintf(stderr, "Invalid max size.\n");
				return false;
			}

			if (pack_mode(params.mode) == -1)
			{
				fprintf(stderr, "Invalid packing mode.\n");
				return false;
			}

			if (has_max_size() && params.pot)
			{
				int w = 1;
				while (w <= params.max_width) w <<= 1;

				int h = 1;
				while (h <= params.max_height) h <<= 1;

				params.max_width = w >> 1;
				params.max_height = h >> 1;
			}

			return true;
		}

		void load_file_list(std::istream &input)
		{
			input >> std::noskipws;

			std::istream_iterator<char> beg(input);
			std::istream_iterator<char> end;

			std::vector<char>(beg, end).swap(filenamesbuf);

			if (filenamesbuf.back() != '\n')
				filenamesbuf.push_back('\n');

			bool addnext = true;

			for (size_t i = 0; i < filenamesbuf.size(); i++)
			{
				if (filenamesbuf[i] == '\n' || filenamesbuf[i] == '\r')
				{
					filenamesbuf[i] = '\0';
					addnext = true;
				}
				else if (addnext)
				{
					filenames.push_back(&filenamesbuf[i]);
					addnext = false;
				}
			}
		}

		bool load_sprites_info()
		{
			input_sprites.resize(filenames.size());
			input_rects.resize(filenames.size());

			for (size_t i = 0; i < filenames.size(); i++)
			{
				Sprite &sprite = input_sprites[i];
				rbp::RectSize &rect = input_rects[i];

				sprite.filename = filenames[i];

				if (!png::info(sprite.filename, &sprite.width, &sprite.height))
				{
					fprintf(stderr, "Error reading image info from %s\n", filenames[i]);
					return false;
				}

				rect.width = sprite.width + params.padding;
				rect.height = sprite.height + params.padding;
			}

			int W = 0;
			int H = 0;

			if (has_fixed_size())
			{
				W = params.width;
				H = params.height;
			}
			else if (has_max_size())
			{
				W = params.max_width;
				H = params.max_height;
			}

			if (W > 0 && H > 0)
			{
				for (size_t i = 0; i < input_sprites.size(); i++)
				{
					int w = 2 * params.padding + input_sprites[i].width;
					int h = 2 * params.padding + input_sprites[i].height;

					if (w > W || h > H)
					{
						fputs("Some of the sprites are larger than the allowed size.\n", stderr);
						return false;
					}
				}
			}

			return true;
		}

		bool has_fixed_size()
		{
			return params.width > 0 && params.height > 0;
		}

		bool has_max_size()
		{
			return !has_fixed_size() && params.max_width > 0 && params.max_height > 0;
		}

		bool can_enlarge(int width, int height)
		{
			if (has_fixed_size())
				return false;

			if (!has_max_size())
				return true;

			return width < params.max_width || height < params.max_height;
		}

		void calculate_initial_size(const std::vector<size_t> &rect_indices, int *w, int *h)
		{
			if (has_fixed_size())
			{
				*w = params.width;
				*h = params.height;
				return;
			}

			uint64_t area = 0;

			for (size_t i = 0; i < rect_indices.size(); i++)
			{
				const rbp::RectSize &rc = input_rects[rect_indices[i]];
				area += rc.width * rc.height;
			}

			int n = 1;
			int size = std::ceil(std::sqrt((double)area));

			while (n < size)
				n <<= 1;

			*w = *h = n;

			if (has_max_size())
			{
				*w = std::min(n, params.max_width);
				*h = std::min(n, params.max_height);
			}
		}

		std::vector<Result*> compute_results()
		{
			std::vector<Result*> result;

			int mode = pack_mode(params.mode);

			if (mode == 0)
			{
				int modes[] = {
					rbp::MaxRects::BottomLeft,
					rbp::MaxRects::ShortSide,
					rbp::MaxRects::LongSide,
					rbp::MaxRects::BestArea,
					rbp::MaxRects::ContactPoint
				};

				std::vector<Result*> best;
				uint64_t best_area = 0;

				for (size_t i = 0; i < countof(modes); i++)
				{
					std::vector<Result*> res = compute_result(modes[i]);

					uint64_t area = 0;

					for (size_t j = 0; j < res.size(); j++)
						area += res[j]->width * res[j]->height;

					if (best.size() == 0 || res.size() < best.size() ||
						(res.size() == best.size() && area < best_area))
					{
						for (size_t j = 0; j < best.size(); j++)
							delete best[j];

						best.swap(res);
						best_area = area;
					}
					else
					{
						for (size_t j = 0; j < res.size(); j++)
							delete res[j];
					}
				}

				result.swap(best);
			}
			else
			{
				result = compute_result(mode);
			}

			return result;
		}

		std::vector<Result*> compute_result(int mode)
		{
			std::vector<Result*> results;

			std::vector<rbp::Rect> result_rects;
			std::vector<size_t> result_indices;
			std::vector<size_t> input_indices(input_rects.size());
			std::vector<size_t> rects_indices;

			for (size_t i = 0; i < input_rects.size(); i++)
				input_indices[i] = i;

			int w = 0;
			int h = 0;

			calculate_initial_size(input_indices, &w, &h);

			while (input_indices.size() > 0)
			{
				rects_indices = input_indices;

				rbp::MaxRects packer(w - params.padding, h - params.padding, params.rotate);

				packer.insert(mode, input_rects, rects_indices, result_rects, result_indices);

				bool add_result = false;

				if (rects_indices.size() > 0)
				{
					if (can_enlarge(w, h))
					{
						if (has_max_size())
						{
							int *x = 0;

							if (w > h)
								x = h < params.max_height ? &h : &w;
							else
								x = w < params.max_width ? &w : &h;

							int max = x == &w ? params.max_width : params.max_height;

							*x = std::min(*x * 2, max);
						}
						else
						{
							if (w > h)
								h *= 2;
							else
								w *= 2;
						}
					}
					else
					{
						add_result = true;
						input_indices.swap(rects_indices);
					}
				}
				else
				{
					add_result = true;
					input_indices.clear();
				}

				if (add_result)
				{
					Result *result = new Result();

					result->width = w;
					result->height = h;

					result->sprites.resize(result_rects.size());

					int xmin = w;
					int xmax = 0;
					int ymin = h;
					int ymax = 0;

					for (size_t i = 0; i < result_rects.size(); i++)
					{
						size_t index = result_indices[i];

						const rbp::RectSize &base_rect = input_rects[index];
						const Sprite &base_sprite = input_sprites[index];

						Sprite &sprite = result->sprites[i];

						sprite = base_sprite;
						sprite.x = result_rects[i].x + params.padding;
						sprite.y = result_rects[i].y + params.padding;
						sprite.rotated = (result_rects[i].width != base_rect.width);

						xmin = std::min(xmin, result_rects[i].x);
						xmax = std::max(xmax, result_rects[i].x + result_rects[i].width);
						ymin = std::min(ymin, result_rects[i].y);
						ymax = std::max(ymax, result_rects[i].y + result_rects[i].height);
					}

					if (!has_fixed_size() && !params.pot)
					{
						result->width = (xmax - xmin) + params.padding;
						result->height = (ymax - ymin) + params.padding;

						if (xmin > 0 || ymin > 0)
						{
							for (size_t i = 0; i < result->sprites.size(); i++)
							{
								result->sprites[i].x -= xmin;
								result->sprites[i].y -= ymin;
							}
						}
					}

					results.push_back(result);

					if (input_indices.size() > 0)
						calculate_initial_size(input_indices, &w, &h);
				}
			}

			return results;
		}

		void load_metadata()
		{
			if (params.metadata != 0)
			{
				Json::Reader reader;
				std::ifstream file(params.metadata);

				if (!reader.parse(file, metadata))
					fprintf(stderr, "Failed to load/parse metadata file.\n");
			}
		}

		void create_png_files(const std::vector<Result*> &results)
		{
			char buf[32];
			std::string filename;

			for (size_t i = 0; i < results.size(); i++)
			{
				filename = params.output;

				if (results.size() > 0)
				{
					sprintf(buf, "-%d.png", (int)i);
					filename += buf;
				}
				else
				{
					filename += ".png";
				}

				create_png_file(filename.c_str(), *results[i]);
			}
		}

		uint8_t *load_sprite(const Sprite &sprite, int *channels)
		{
			int width;
			int height;

			uint8_t *data = png::load(sprite.filename, &width, &height, channels);

			if (data == 0 || width != sprite.width || height != sprite.height || *channels < 3)
			{
				fprintf(stderr, "Something is wrong with the image %s\n", sprite.filename);

				delete[] data;
				data = 0;
			}

			return data;
		}

		void create_png_file(const char *filename, const Result &result)
		{
			std::vector<uint8_t> dstbuffer(4 * result.width * result.height);
			std::vector<uint8_t> srcbuffer;

			const int w = result.width;
			const int h = result.height;
			const int dstpitch = w * 4;

			for (size_t i = 0; i < result.sprites.size(); i++)
			{
				const Sprite &sprite = result.sprites[i];

				int channels;
				const uint8_t *data = load_sprite(sprite, &channels);

				if (data == 0)
				{
					channels = 4;
					srcbuffer.resize(channels * sprite.width * sprite.height);
					data = &srcbuffer[0];
				}

				const int srcpitch = sprite.width * channels;
				const uint8_t *src = data;

				if (sprite.rotated)
				{
					// top-left of the sprite will be at top-right in the spritesheet

					uint8_t *dst = &dstbuffer[4 * (sprite.y * w + sprite.x + sprite.height - 1)];

					const int nextcol = -(sprite.width * dstpitch + 4);

					for (int y = 0; y < sprite.height; y++)
					{
						for (int x = 0; x < srcpitch; x += channels)
						{
							dst[0] = src[x + 0];
							dst[1] = src[x + 1];
							dst[2] = src[x + 2];
							dst[3] = channels == 4 ? src[x + 3] : 0xFF;

							dst += dstpitch;
						}

						src += srcpitch;
						dst += nextcol;
					}
				}
				else
				{
					uint8_t *dst = &dstbuffer[4 * (sprite.y * w + sprite.x)];

					if (channels == 4)
					{
						for (int y = 0; y < sprite.height; y++)
						{
							memcpy(dst, src, srcpitch);

							src += srcpitch;
							dst += dstpitch;
						}
					}
					else if (channels == 3)
					{
						for (int y = 0; y < sprite.height; y++)
						{
							for (int xs = 0, xd = 0; xs < srcpitch; xs += 3, xd += 4)
							{
								dst[xd + 0] = src[xs + 0];
								dst[xd + 1] = src[xs + 1];
								dst[xd + 2] = src[xs + 2];
								dst[xd + 3] = 0xFF;
							}

							src += srcpitch;
							dst += dstpitch;
						}
					}
				}

				if (data != &srcbuffer[0])
					delete[] data;
			}

			if (params.bleed)
				bleed_apply(&dstbuffer[0], w, h);

			png::save(filename, w, h, &dstbuffer[0]);
		}

		void create_json_files(const std::vector<Result*> &results)
		{
			char buf[32];
			std::string filename;

			for (size_t i = 0; i < results.size(); i++)
			{
				filename = params.output;

				if (results.size() > 0)
				{
					sprintf(buf, "-%d.json", (int)i);
					filename += buf;
				}
				else
				{
					filename += ".json";
				}

				create_json_file(filename.c_str(), *results[i]);
			}
		}

		void create_json_file(const char *filename, const Result &result)
		{
			std::ofstream json(filename, std::ofstream::trunc);

			if (!json.is_open())
			{
				fprintf(stderr, "Error creating file %s\n", filename);
				return;
			}

			Json::Value root(Json::objectValue);

			root["width"] = result.width;
			root["height"] = result.height;
			root["sprites"] = Json::Value(Json::objectValue);

			for (size_t i = 0; i < result.sprites.size(); i++)
			{
				const Sprite &sprite = result.sprites[i];

				Json::Value info;

				if (metadata.isObject() && metadata.isMember(sprite.filename))
					info = metadata[sprite.filename];

				info["w"] = sprite.width;
				info["h"] = sprite.height;
				info["tl"] = Json::Value(Json::objectValue);
				info["br"] = Json::Value(Json::objectValue);

				if (sprite.rotated)
				{
					info["tl"]["x"] = sprite.x + sprite.height;
					info["tl"]["y"] = sprite.y;
					info["br"]["x"] = sprite.x;
					info["br"]["y"] = sprite.y + sprite.width;
				}
				else
				{
					info["tl"]["x"] = sprite.x;
					info["tl"]["y"] = sprite.y;
					info["br"]["x"] = sprite.x + sprite.width;
					info["br"]["y"] = sprite.y + sprite.height;
				}

				root["sprites"][sprite.filename] = info;
			}

			if (params.pretty)
			{
				Json::StyledStreamWriter writer(params.indentation);
				writer.write(json, root);
			}
			else
			{
				Json::FastWriter writer;
				json << writer.write(root);
			}
		}
	};

	struct c_string
	{
		char *buffer;
		c_string(const char *str) : buffer(new char[strlen(str) + 1]) { strcpy(buffer, str); }
		~c_string() { delete[] buffer; }
		operator char*&() { return buffer; }
	};

	static bool is_dir(const char *path)
	{
		struct stat sb;
		return stat(path, &sb) == 0 && S_ISDIR(sb.st_mode);
	}

	static bool create_dir(const char *path)
	{
		if (!is_dir(path))
		{
			if (create_dir(dirname(c_string(path))))
				mkdir(path, S_IRWXU);

			return is_dir(path);
		}

		return true;
	}

	int pack(std::istream &input, const Params &params)
	{
		Packer packer(params);

		if (!packer.validate_params())
			return 1;

		if (!create_dir(dirname(c_string(params.output))))
		{
			fputs("Failed to create directory.\n", stderr);
			return 1;
		}

		packer.load_file_list(input);

		if (!packer.load_sprites_info())
			return 1;

		packer.load_metadata();

		std::vector<Result*> results = packer.compute_results();

		packer.create_png_files(results);
		packer.create_json_files(results);

		for (size_t i = 0; i < results.size(); i++)
			delete results[i];

		return 0;
	}
}
