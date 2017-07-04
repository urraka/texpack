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
			format("legacy"),
			bleed(false),
			premultiplied(false),
			pot(false),
			rotate(false),
			pretty(false),
			trim(false),
			indentation(0),
			padding(0),
			width(0),
			height(0),
			max_size(false)
		{}

		const char *output;
		const char *metadata;
		const char *mode;
		const char *format;
		bool bleed;
		bool premultiplied;
		bool pot;
		bool rotate;
		bool pretty;
		bool trim;
		int indentation;
		int padding;
		int width;
		int height;
		bool max_size;
	};

	int pack(std::istream &input, const Params &params);
}
