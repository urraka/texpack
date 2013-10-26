#pragma once
#include <istream>

namespace pkr
{
	void pack(std::istream &input, const char *output, int padding = 1);
}
