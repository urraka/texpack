#include "packer.h"
#include "bleeding.h"
#include "png.h"
#include "rbp/MaxRectsBinPack.h"
#include "json/json.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <iterator>
#include <cmath>
#include <stdint.h>
#include <assert.h>

#define countof(x) (sizeof(x) / sizeof(x[0]))

namespace pkr
{
	struct Sprite
	{
		const char *filename;
		int x;
		int y;
		int width;
		int height;
		int channels;
		bool rotated;
		uint8_t *data;
	};

	struct ImageData
	{
		ImageData() : data(0) {}
		~ImageData() { if (data) delete[] data; }
		uint8_t *data;
	};

	struct Result
	{
		int width;
		int height;
		std::vector<Sprite> sprites;
	};

	void pack(std::istream &input, const Params &params)
	{
		// read all filenames from input

		std::vector<char*> filenames;

		input >> std::noskipws;

		std::istream_iterator<char> input_begin(input);
		std::istream_iterator<char> input_end;

		std::vector<char> fnamebuf(input_begin, input_end);

		if (fnamebuf.back() != '\n')
			fnamebuf.push_back('\n');

		bool addnext = true;

		for (size_t i = 0; i < fnamebuf.size(); i++)
		{
			if (fnamebuf[i] == '\n' || fnamebuf[i] == '\r')
			{
				fnamebuf[i] = '\0';
				addnext = true;
			}
			else if (addnext)
			{
				filenames.push_back(&fnamebuf[i]);
				addnext = false;
			}
		}

		// load the input images

		const int N = filenames.size();

		std::vector<Sprite> sprites(N);
		std::vector<ImageData> imageData(N);
		std::vector<rbp::RectSize> rects(N);

		uint64_t area = 0;

		for (int i = 0; i < N; i++)
		{
			Sprite &sprite = sprites[i];

			sprite.filename = filenames[i];
			sprite.data = png::load(sprite.filename, &sprite.width, &sprite.height, &sprite.channels);

			if (!sprite.data || sprite.channels < 3)
			{
				std::cout << "error: \"" << filenames[i] << "\" is not a valid image file." << std::endl;
				return;
			}

			imageData[i].data = sprite.data;

			rects[i].width = sprite.width + params.padding;
			rects[i].height = sprite.height + params.padding;

			area += (sprite.width + params.padding) * (sprite.height + params.padding);
		}

		// find a starting size for the final image

		int width = 0;
		int height = 0;

		{
			int n = 1;
			int size = std::ceil(std::sqrt((float)area));

			while (n < size)
				n <<= 1;

			width = n;
			height = n;
		}

		// find a bunch of possible results

		rbp::MaxRectsBinPack::FreeRectChoiceHeuristic modes[] = {
			rbp::MaxRectsBinPack::RectBestShortSideFit,
			rbp::MaxRectsBinPack::RectBestLongSideFit,
			rbp::MaxRectsBinPack::RectBestAreaFit,
			rbp::MaxRectsBinPack::RectBottomLeftRule,
			rbp::MaxRectsBinPack::RectContactPointRule
		};

		std::vector<rbp::Rect> resultRects(N);
		std::vector<int> resultIndices(N);
		std::vector<Result*> results(countof(modes));

		for (size_t i = 0; i < countof(modes); i++)
		{
			int w = width;
			int h = height;

			while (true)
			{
				rbp::MaxRectsBinPack binpack(w - params.padding, h - params.padding);

				if (binpack.Insert(rects, resultRects, resultIndices, modes[i]))
				{
					assert(rects.size() == resultRects.size() && rects.size() == resultIndices.size());

					Result *result = new Result();

					result->width = w;
					result->height = h;

					result->sprites.resize(N);

					for (int j = 0; j < N; j++)
					{
						const rbp::RectSize &a = rects[resultIndices[j]];
						const rbp::Rect &b = resultRects[j];

						Sprite &s = result->sprites[resultIndices[j]];

						s = sprites[resultIndices[j]];
						s.x = b.x + params.padding;
						s.y = b.y + params.padding;
						s.rotated = true;

						if (a.width == b.width && a.height == b.height)
							s.rotated = false;
					}

					results[i] = result;

					break;
				}
				else
				{
					if (w > h)
						h *= 2;
					else
						w *= 2;
				}
			}
		}

		// load metadata if present

		Json::Value metadata;

		if (params.metadata != 0)
		{
			Json::Reader reader;
			std::ifstream file(params.metadata);

			if (!reader.parse(file, metadata))
			{
				std::cout << "Metadata failed to load (" << params.metadata << ")" << std::endl;
				std::cout << reader.getFormattedErrorMessages() << std::endl;
			}
		}

		// generate images

		const char *modeNames[] = {
			"BestShortSideFit",
			"BestLongSideFit",
			"BestAreaFit",
			"BottomLeftRule",
			"ContactPointRule"
		};

		size_t bufferSize = 0;

		for (size_t i = 0; i < results.size(); i++)
			bufferSize = std::max(bufferSize, (size_t)(results[i]->width * results[i]->height * 4));

		uint8_t *imageBuffer = new uint8_t[bufferSize];

		for (size_t i = 0; i < results.size(); i++)
		{
			// build & save png

			const int w = results[i]->width;
			const int h = results[i]->height;

			const int dstpitch = w * 4;

			std::memset(imageBuffer, 0, w * h * 4);

			for (size_t j = 0; j < results[i]->sprites.size(); j++)
			{
				const Sprite &sprite = results[i]->sprites[j];

				const int srcpitch = sprite.width * sprite.channels;
				uint8_t *src = sprite.data;

				if (sprite.rotated)
				{
					// top-left of the sprite will be at top-right in the spritesheet

					uint8_t *dst = &imageBuffer[4 * (sprite.y * w + sprite.x + sprite.height - 1)];

					const int nextcol = -(sprite.width * dstpitch + 4);

					for (int y = 0; y < sprite.height; y++)
					{
						for (int x = 0; x < srcpitch; x += sprite.channels)
						{
							dst[0] = src[x + 0];
							dst[1] = src[x + 1];
							dst[2] = src[x + 2];
							dst[3] = sprite.channels == 4 ? src[x + 3] : 0xFF;

							dst += dstpitch;
						}

						src += srcpitch;
						dst += nextcol;
					}
				}
				else
				{
					uint8_t *dst = &imageBuffer[4 * (sprite.y * w + sprite.x)];

					if (sprite.channels == 4)
					{
						for (int y = 0; y < sprite.height; y++)
						{
							memcpy(dst, src, srcpitch);

							src += srcpitch;
							dst += dstpitch;
						}
					}
					else if (sprite.channels == 3)
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
			}

			if (params.bleed > 0)
				bleed_apply(imageBuffer, w, h, params.bleed);

			std::string filename(params.output);
			filename += '-';
			filename += modeNames[i];

			png::save((filename + ".png").c_str(), w, h, imageBuffer);

			// generate json file

			std::ofstream json((filename + ".json").c_str(), std::ofstream::trunc);

			if (json.is_open())
			{
				Json::Value root(Json::objectValue);

				root["width"] = w;
				root["height"] = h;
				root["sprites"] = Json::Value(Json::objectValue);

				for (size_t j = 0; j < results[i]->sprites.size(); j++)
				{
					Sprite &sprite = results[i]->sprites[j];

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

				Json::FastWriter writer;

				json << writer.write(root);
			}

			delete results[i];
		}

		delete[] imageBuffer;
	}
}
