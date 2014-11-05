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
			indentation("\t"),
			bleed(false),
			pot(false),
			rotate(false),
			pretty(false),
			padding(0),
			width(0),
			height(0),
			max_width(0),
			max_height(0)
		{}

		const char *output;
		const char *metadata;
		const char *mode;
		const char *indentation;
		bool bleed;
		bool pot;
		bool rotate;
		bool pretty;
		int padding;
		int width;
		int height;
		int max_width;
		int max_height;
	};

	int pack(std::istream &input, const Params &params);
}
