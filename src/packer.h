#pragma once
#include <istream>

namespace pkr
{
	struct Params
	{
		Params() : output(0), metadata(0), padding(0), bleed(false) {}

		const char *output;
		const char *metadata;
		int padding;
		bool bleed;
	};

	void pack(std::istream &input, const Params &params);
}
