#include "packer.h"
#include "bleeding.h"
#include "png/png.h"
#include "rbp/MaxRects.h"

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"

#include "sebclaeys_xml/XMLWriter.hh"

#include <algorithm>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <iterator>
#include <cmath>
#include <cstdio>
#include <stdint.h>
#include <sys/stat.h>

#if !defined(_MSC_VER)
#include <libgen.h>
#include <unistd.h>
#endif

#define countof(x) (sizeof(x) / sizeof(x[0]))

#if defined(_WIN32) && !defined(_MSC_VER)
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

	int real_width;
	int real_height;
	int xoffset;
	int yoffset;
};

struct Result
{
	int width;
	int height;
	std::vector<Sprite> sprites;
};

struct Packer
{
	int formatting;

	Params params;

	std::vector<char*> filenames;
	std::vector<char> filenamesbuf;

	std::vector<Sprite> input_sprites;
	std::vector<rbp::RectSize> input_rects;

	rapidjson::Document metadata;

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

	int format_mode(const char *mode)
	{
		static const char *modes[] = {
			"jsonarray",
			"jsonhash",
			"legacy",
			"xml"
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
			fputs("The argument --output is required.\n", stderr);
			return false;
		}

		if (params.padding < 0)
		{
			fputs("Invalid padding.\n", stderr);
			return false;
		}

		if ((params.width > 0 || params.height > 0 || params.max_size) &&
			(params.width <= 0 || params.height <= 0))
		{
			fputs("Invalid size.\n", stderr);
			return false;
		}

		if (pack_mode(params.mode) == -1)
		{
			fputs("Invalid packing mode.\n", stderr);
			return false;
		}

		formatting = format_mode(params.format);
		if (formatting == -1)
		{
			fputs("Invalid format mode.\n", stderr);
			return false;
		}

		if (params.max_size && params.pot)
		{
			int w = 1;
			while (w <= params.width) w <<= 1;

			int h = 1;
			while (h <= params.height) h <<= 1;

			params.width = w >> 1;
			params.height = h >> 1;
		}

		if (params.indentation < 0)
		{
			fputs("Invalid indentation. Using default.\n", stderr);
			params.indentation = 0;
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

		// Cycles through the list of file names
		for (size_t i = 0; i < filenamesbuf.size(); i++)
		{
			// If a new line
			if (filenamesbuf[i] == '\n' || filenamesbuf[i] == '\r')
			{
				// clears the current line
				filenamesbuf[i] = '\0';
				// Add the next, as it's the data we want
				addnext = true;
			}
			else if (addnext)
			{
				filenames.push_back(&filenamesbuf[i]);
				addnext = false;
			}
		}

		// Cycle through all the loaded filenames and generate numbers
		// e.g. sauce[01-03].png will generate sauce01.png sauce02.png and sauce03.png
		for (size_t i = 0; i < filenames.size(); i++)
		{
			std::string name(filenames[i]);

			std::size_t open_bracket = name.find_last_of("[");
			std::size_t close_bracket = name.find_last_of("]");

			// If there is both an open and close square bracket []
			if (open_bracket != std::string::npos && close_bracket != std::string::npos)
			{
				std::size_t dir_marker = name.find_last_of("/\\");
				std::size_t file_extension = name.find_last_of(".");

				// If there are no directory markings OR if there are, that the square bracket comes after all of them
				// And if there is no extension OR if there is, that the square bracket comes before it
				// And the brackets are in the proper order (open then closed)
				if ((dir_marker == std::string::npos || dir_marker < open_bracket) && (file_extension == std::string::npos || file_extension > close_bracket) && open_bracket < close_bracket)
				{
					std::size_t numbers_length = close_bracket - open_bracket - 1;
					std::string numbers = name.substr(open_bracket + 1, numbers_length);

					// If everything is a valid number (or a hyphen)
					if (numbers.find_first_not_of("0123456789-") == std::string::npos){

						int hyphen = numbers.find_first_of("-");

						// And there's one "-" (hyphen)
						if (hyphen == (int) numbers.find_last_of("-") && hyphen != (int) std::string::npos)
						{
							// The minimum length of the number should be the length of the first number given, therefore the value of hyphen
							std::string first = numbers.substr(0, hyphen);
							std::string second = numbers.substr(hyphen + 1, numbers_length - hyphen - 1);

							int first_num = std::stoi(first);
							int second_num = std::stoi(second);

							if (first_num < second_num)
							{
								// Removes the original filename
								filenames.erase(filenames.begin() + i);

								std::string filename_start = name.substr(0, open_bracket);
								std::string filename_end = name.substr(close_bracket + 1);

								for (int i = first_num; i <= second_num; i++)
								{
									std::string number_string = std::to_string(i);
									int num_str_length = number_string.length();
									int total_length = hyphen - num_str_length; // hyphen is the minimum length

									// Add zeros to the beginning of the string until the length of the string is the minimum
									for (int j = 0; j < total_length; j++)
									{
										number_string = "0" + number_string;
									}

									std::string new_filename = filename_start + number_string + filename_end;

									// Converts the string to s char*
									char * writable = new char[new_filename.size() + 1];
									std::copy(new_filename.begin(), new_filename.end(), writable);
									writable[new_filename.size()] = '\0'; // Terminate

									// Adds the entry to the list
									filenames.push_back(writable);
								}
							}
							else
							{
								fprintf(stderr, "Invalid bracket numbers %s\n", filenames[i]);
							}
						}
						else
						{
							fprintf(stderr, "Invalid hyphen in brackets %s\n", filenames[i]);
						}
					}
					else
					{
						fprintf(stderr, "Invalid character in brackets, only accepts numbers and a hyphen %s\n", filenames[i]);
					}
				}
				else
				{
					fprintf(stderr, "Invalid bracket formatting %s\n", filenames[i]);
				}
			}
		}
	}

	bool read_trim_metrics(Sprite *sprite)
	{
		int w, h, channels;

		uint8_t *data = png::load(sprite->filename, &w, &h, &channels);

		if (data == 0)
			return false;

		sprite->xoffset = 0;
		sprite->yoffset = 0;
		sprite->width = w;
		sprite->height = h;
		sprite->real_width = w;
		sprite->real_height = h;

		if (channels != 4)
			return delete[] data, true;

		int i, l, t, r, b; // left, top, right, bottom
		int stride = 4 * w;

		// top
		for (t = 0, i = 3; t < h; t++)
		{
			uint8_t value = 0;

			for (int x = 0; x < w; x++, i += 4)
				value |= data[i];

			if (value != 0)
				break;
		}

		if (t == h) // fully transparent image, keep it 1x1
		{
			sprite->width = 1;
			sprite->height = 1;
			return delete[] data, true;
		}

		// bottom
		for (b = h - 1, i = b * stride + 3; b > t; b--, i -= stride)
		{
			uint8_t value = 0;

			for (int x = 0, j = i; x < w; x++, j += 4)
				value |= data[j];

			if (value != 0)
				break;
		}

		// left
		for (l = 0, i = t * stride + 3; l < w; l++, i += 4)
		{
			uint8_t value = 0;

			for (int y = t, j = i; y <= b; y++, j += stride)
				value |= data[j];

			if (value != 0)
				break;
		}

		// right
		for (r = w - 1, i = (t + 1) * stride - 1; r > l; r--, i -= 4)
		{
			uint8_t value = 0;

			for (int y = t, j = i; y <= b; y++, j += stride)
				value |= data[j];

			if (value != 0)
				break;
		}

		sprite->xoffset = l;
		sprite->yoffset = t;
		sprite->width = (r - l) + 1;
		sprite->height = (b - t) + 1;

		return delete[] data, true;
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

			if (params.trim)
			{
				if (!read_trim_metrics(&sprite))
				{
					fprintf(stderr, "Error reading image %s\n", filenames[i]);
					return false;
				}
			}
			else
			{
				if (!png::info(sprite.filename, &sprite.real_width, &sprite.real_height))
				{
					fprintf(stderr, "Error reading image info from %s\n", filenames[i]);
					return false;
				}

				sprite.xoffset = 0;
				sprite.yoffset = 0;
				sprite.width = sprite.real_width;
				sprite.height = sprite.real_height;
			}

			rect.width = sprite.width + params.padding;
			rect.height = sprite.height + params.padding;
		}

		if (params.width > 0 && params.height > 0)
		{
			for (size_t i = 0; i < input_sprites.size(); i++)
			{
				int w = 2 * params.padding + input_sprites[i].width;
				int h = 2 * params.padding + input_sprites[i].height;

				if (w > params.width || h > params.height)
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
		return params.width > 0 && params.height > 0 && !params.max_size;
	}

	bool can_enlarge(int width, int height)
	{
		return !has_fixed_size() && (!params.max_size ||
			width < params.width || height < params.height);
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

		if (params.max_size)
		{
			*w = std::min(n, params.width);
			*h = std::min(n, params.height);
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
					if (params.max_size)
					{
						int *x = 0;

						if (w > h)
							x = h < params.height ? &h : &w;
						else
							x = w < params.width ? &w : &h;

						int max = x == &w ? params.width : params.height;

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
			FILE *file = fopen(params.metadata, "rb");

			if (file == 0)
			{
				fprintf(stderr, "Failed to load metadata file (%s).\n", params.metadata);
				return;
			}

			using namespace rapidjson;

			char buffer[4096];
			FileReadStream stream(file, buffer, sizeof(buffer));
			metadata.ParseStream(stream);
			fclose(file);

			if (metadata.HasParseError() || !metadata.IsObject())
			{
				fputs("Invalid metadata file.\n", stderr);
				metadata.SetNull();
			}
		}
	}

	void create_png_files(const std::vector<Result*> &results)
	{
		char buf[32];
		std::string filename;

		for (size_t i = 0; i < results.size(); i++)
		{
			filename = params.output;

			if (results.size() > 1)
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

		if (data == 0 || width != sprite.real_width || height != sprite.real_height || *channels < 3)
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
				srcbuffer.resize(channels * sprite.real_width * sprite.real_height);
				data = &srcbuffer[0];
			}

			const int x0 = sprite.xoffset;
			const int y0 = sprite.yoffset;
			const int srcpitch = sprite.real_width * channels;
			const uint8_t *src = &data[y0 * srcpitch + x0 * channels];

			if (sprite.rotated)
			{
				// top-left of the sprite will be at top-right in the spritesheet

				uint8_t *dst = &dstbuffer[4 * (sprite.y * w + sprite.x + sprite.height - 1)];

				const int nextcol = -(sprite.width * dstpitch + 4);

				for (int y = 0; y < sprite.height; y++)
				{
					const uint8_t *s = src;

					for (int x = 0; x < sprite.width; x++)
					{
						dst[0] = *s++;
						dst[1] = *s++;
						dst[2] = *s++;
						dst[3] = channels == 4 ? *s++ : 0xFF;

						dst += dstpitch;
					}

					src += srcpitch;
					dst += nextcol;
				}
			}
			else
			{
				const int row_size = channels * sprite.width;
				uint8_t *dst = &dstbuffer[4 * (sprite.y * w + sprite.x)];

				if (channels == 4)
				{
					for (int y = 0; y < sprite.height; y++)
					{
						memcpy(dst, src, row_size);

						src += srcpitch;
						dst += dstpitch;
					}
				}
				else if (channels == 3)
				{
					for (int y = 0; y < sprite.height; y++)
					{
						for (int xs = 0, xd = 0; xs < row_size; xs += 3, xd += 4)
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

		if (params.premultiplied)
		{
			for (uint8_t *p = &dstbuffer[0]; p < &dstbuffer.back(); p++)
			{
				float alpha = p[3] / 255.f;

				*p++ *= alpha;
				*p++ *= alpha;
				*p++ *= alpha;
			}
		}

		png::save(filename, w, h, &dstbuffer[0]);
	}

	void create_files(const std::vector<Result*> &results)
	{
		char buf[32];
		std::string filename;

		for (size_t i = 0; i < results.size(); i++)
		{
			filename = params.output;

			if (results.size() > 1)
			{
				// XML formatting
				if (formatting == 3)
					sprintf(buf, "-%d.xml", (int)i);
				else
					sprintf(buf, "-%d.json", (int)i);

				filename += buf;
			}
			else
			{
				filename += formatting == 3 ? ".xml" : ".json";
			}

			create_file(filename.c_str(), *results[i]);
		}
	}

	void create_file(const char *filename, const Result &result)
	{
		// XML formatting
		if (formatting == 3)
		{
			std::ofstream stream2;
			stream2.open(filename);

			XMLWriter writer(stream2);
			write_xml(result, writer, filename);
		}
		else
		{
			FILE *file = fopen(filename, "wb");

			if (file == 0)
			{
				fprintf(stderr, "Error creating file %s\n", filename);
				return;
			}

			using namespace rapidjson;

			char buffer[4096];
			FileWriteStream stream(file, buffer, sizeof(buffer));

			// Setup JSON writer
			if (params.pretty)
			{
				PrettyWriter<FileWriteStream> writer(stream);

				if (params.indentation > 0)
					writer.SetIndent(' ', params.indentation);
				else
					writer.SetIndent('\t', 1);

				write_json(result, writer, filename);
			}
			else
			{
				Writer<FileWriteStream> writer(stream);
				write_json(result, writer, filename);
			}
			fclose(file);
		}
	}

	void write_xml(const Result &result, XMLWriter writer, const char *filename)
	{
		writer.content("<!-- Created with TexPack https://github.com/urraka/texpack -->\n");

		writer.openElt("TextureAtlas");
		writer.attr("imagePath", format_meta_image_name(filename));
		writer.attr("width", result.width);
		writer.attr("height", result.height);

		for (size_t i = 0; i < result.sprites.size(); i++)
		{
			const Sprite &sprite = result.sprites[i];

			writer.openElt("SubTexture");

			writer.attr("name", remove_extension(sprite.filename));
			writer.attr("x", sprite.x);
			writer.attr("y", sprite.y);

			if (sprite.rotated) // By default rotated is false in xml loaders, so don't include it unless it's neccessary
				writer.attr("rotated", "true");

			writer.attr("width", sprite.rotated ? sprite.height : sprite.width);
			writer.attr("height", sprite.rotated ? sprite.width : sprite.height);

			if (params.trim){
				writer.attr("frameX", sprite.xoffset);
				writer.attr("frameY", sprite.yoffset);
				writer.attr("frameWidth", sprite.real_width);
				writer.attr("frameHeight", sprite.real_height);
			}

			writer.closeElt();
		}

		writer.closeAll();
	}

	template<typename T>
	void write_json(const Result &result, T &writer, const char *filename)
	{
		// 0 = jsonarray
		// 1 = jsonhash
		// 2 = legacy
		// 3 = xml

		if (formatting == 0)
		{
			writer.StartObject();

			writer.String("frames");
			writer.StartArray();

			for (size_t i = 0; i < result.sprites.size(); i++)
			{
				const Sprite &sprite = result.sprites[i];

				writer.StartObject();

				writer.String("filename");
				writer.Key(remove_extension(sprite.filename).c_str());

				fill_object_info(writer, sprite);
				writer.EndObject();
			}

			writer.EndArray();
			fill_meta_info(result, writer, filename);
			writer.EndObject();
		}
		else if (formatting == 1)
		{
			writer.StartObject();

			writer.String("frames");
			writer.StartObject();

			for (size_t i = 0; i < result.sprites.size(); i++)
			{
				const Sprite &sprite = result.sprites[i];

				writer.String(remove_extension(sprite.filename).c_str());
				writer.StartObject();

				fill_object_info(writer, sprite);
				writer.EndObject();
			}

			writer.EndObject();
			fill_meta_info(result, writer, filename);
			writer.EndObject();

		}
		else if (formatting == 2)
		{
			writer.StartObject();

			writer.String("width");
			writer.Int(result.width);

			writer.String("height");
			writer.Int(result.height);

			if (metadata.IsObject())
			{
				rapidjson::Value::ConstMemberIterator it = metadata.FindMember(".global");

				if (it != metadata.MemberEnd() && it->value.IsObject())
				{
					writer.String("meta");
					it->value.Accept(writer);
				}
			}

			writer.String("sprites");
			writer.StartObject();

			for (size_t i = 0; i < result.sprites.size(); i++)
			{
				const Sprite &sprite = result.sprites[i];

				writer.String(sprite.filename);
				writer.StartObject();

				writer.String("x");
				writer.Int(sprite.x);

				writer.String("y");
				writer.Int(sprite.y);

				writer.String("width");
				writer.Int(sprite.rotated ? sprite.height : sprite.width);
				writer.String("height");
				writer.Int(sprite.rotated ? sprite.width : sprite.height);

				if (params.rotate)
				{
					writer.String("rotated");
					writer.Bool(sprite.rotated);
				}

				if (params.trim)
				{
					writer.String("real_width");
					writer.Int(sprite.real_width);

					writer.String("real_height");
					writer.Int(sprite.real_height);

					writer.String("xoffset");
					writer.Int(sprite.xoffset);

					writer.String("yoffset");
					writer.Int(sprite.yoffset);
				}

				if (metadata.IsObject())
				{
					rapidjson::Value::ConstMemberIterator it = metadata.FindMember(sprite.filename);

					if (it != metadata.MemberEnd())
					{
						writer.String("meta");
						it->value.Accept(writer);
					}
				}

				writer.EndObject();
			}

			writer.EndObject();
			writer.EndObject();
		}
	}

	std::string remove_extension(const char *filename)
	{
		std::string working_name = filename;
		std::size_t last_index = working_name.find_last_of(".");
		std::string name = working_name.substr(0, last_index);

		return name;
	}

	template<typename T>
	void fill_object_info(T &writer, const Sprite &sprite)
	{
		writer.String("frame");
		writer.StartObject();

		writer.String("x");
		writer.Int(sprite.x);

		writer.String("y");
		writer.Int(sprite.y);

		writer.String("w");
		writer.Int(sprite.rotated ? sprite.height : sprite.width);

		writer.String("h");
		writer.Int(sprite.rotated ? sprite.width : sprite.height);

		writer.EndObject();

		if (sprite.rotated)
		{
			writer.String("rotated");
			writer.Bool(sprite.rotated);
		}

		if (params.trim)
		{
			writer.String("trimmed");
			writer.Bool(params.trim);

			writer.String("spriteSourceSize");
			writer.StartObject();

			writer.String("x");
			writer.Int(sprite.xoffset);

			writer.String("y");
			writer.Int(sprite.yoffset);

			writer.String("w");
			writer.Int(sprite.rotated ? sprite.height : sprite.width);

			writer.String("h");
			writer.Int(sprite.rotated ? sprite.width : sprite.height);

			writer.EndObject();

			writer.String("sourceSize");
			writer.StartObject();

			writer.String("w");
			writer.Int(sprite.rotated ? sprite.real_height : sprite.real_width);

			writer.String("h");
			writer.Int(sprite.rotated ? sprite.real_width : sprite.real_height);

			writer.EndObject();
		}

		if (metadata.IsObject())
		{
			rapidjson::Value::ConstMemberIterator it = metadata.FindMember(sprite.filename);

			if (it != metadata.MemberEnd())
			{
				writer.String("meta");
				it->value.Accept(writer);
			}
		}
	}

	std::string format_meta_image_name(const char *filename)
	{
		// Removes the extension and path, then adds .png back to the file namehub
		std::string working_name = remove_extension(filename);
		std::size_t last_index = working_name.find_last_of("/\\");
		std::string name = working_name.substr(last_index + 1) + ".png";

		return name;
	}

	template<typename T>
	void fill_meta_info(const Result &result, T &writer, const char *filename)
	{
		writer.String("meta");
		writer.StartObject();

		writer.String("app");
		writer.Key("https://github.com/urraka/texpack");

		writer.String("image");
		writer.Key(format_meta_image_name(filename).c_str());

		writer.String("size");
		writer.StartObject();

		writer.String("w");
		writer.Int(result.width);

		writer.String("h");
		writer.Int(result.height);

		writer.EndObject();

		if (metadata.IsObject())
		{
			rapidjson::Value::ConstMemberIterator it = metadata.FindMember(".global");

			if (it != metadata.MemberEnd() && it->value.IsObject())
			{
				rapidjson::Value::ConstMemberIterator end = it->value.MemberEnd();

				it = it->value.MemberBegin();

				while (it != end)
				{
					it->name.Accept(writer);
					it->value.Accept(writer);
					++it;
				}
			}
		}

		writer.EndObject();
	}
};

struct c_string
{
	char *buffer;
	c_string(const char *str) : buffer(new char[strlen(str) + 1]) { strcpy(buffer, str); }
	~c_string() { delete[] buffer; }
	operator char*&() { return buffer; }
};


#if defined(_MSC_VER)
#include <Windows.h>

/// IMPORTANT: The output buffer must be generously sized.
/// Returns number of wide-characters written or 0 on error
static inline size_t
utf8_to_windows_string(const char* input, WCHAR* output, size_t output_char_capacity)
{
    size_t result = (size_t)
      MultiByteToWideChar(CP_UTF8, 0, input, -1, output, (int)output_char_capacity);
    assert(result > 0);
    return result;
}

/// This expects a UTF-8 encoded path that is closed with a file separator
/// Original code source (https://stackoverflow.com/a/16719260)
static bool
create_dir(const char* rootpath)
{
    wchar_t rootpath_win[MAX_PATH];
    utf8_to_windows_string(rootpath, rootpath_win, MAX_PATH);

    wchar_t folder[MAX_PATH];
    ZeroMemory(folder, MAX_PATH * sizeof(wchar_t));

    bool last_creation_sucess = true;
    wchar_t* end = wcschr(rootpath_win, L'\\');
    while (end != NULL)
    {
        wcsncpy(folder, rootpath_win, end - rootpath_win + 1);
        if (CreateDirectoryW(folder, NULL))
        {
            last_creation_sucess = true;
        }
        else
        {
            DWORD err = GetLastError();
            if (err != ERROR_ALREADY_EXISTS)
            {
                last_creation_sucess = false;
            }
        }
        end = wcschr(++end, L'\\');
    }

    return last_creation_sucess;
}

static char* dirname(char* path)
{
    // NOTE: This should work with the above `create_dir` function
    size_t length = strlen(path);
    for (int index = (int)length; index >= 0; index--)
    {
        if ((path[index] == '/') || (path[index] == '\\'))
        {
            path[index] = '\\';
            path[index+1] = '\0';
            break;;
        }
    }
    return path;
}



#else

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

#endif


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

	std::vector<Result*> results = packer.compute_results();

	packer.create_png_files(results);

	packer.load_metadata();

	packer.create_files(results);

	for (size_t i = 0; i < results.size(); i++)
		delete results[i];

	return 0;
}

}
