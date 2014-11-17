#pragma once
#include <istream>

namespace pkr
{
	struct Params
	{
		Params():
			output(0),
			metadata(0),
			mode("auto"),
			bleed(false),
			premultiplied(false),
			pot(false),
			rotate(false),
			pretty(false),
			indentation(0),
			padding(0),
			width(0),
			height(0),
			max_width(0),
			max_height(0)
		{}

		const char *output;
		const char *metadata;
		const char *mode;
		bool bleed;
		bool premultiplied;
		bool pot;
		bool rotate;
		bool pretty;
		int indentation;
		int padding;
		int width;
		int height;
		int max_width;
		int max_height;
	};

	int pack(std::istream &input, const Params &params);
}
