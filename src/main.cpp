#include "packer.h"

#include <iostream>
#include <cstring>
#include <cstdlib>

int main(int argc, char *argv[])
{
	int output   = -1;
	int padding  = -1;
	int metadata = -1;
	int bleed    = -1;

	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-' && strlen(argv[i]) == 2)
		{
			switch (argv[i][1])
			{
				case 'p': if (++i < argc) padding  = i; break;
				case 'o': if (++i < argc) output   = i; break;
				case 'm': if (++i < argc) metadata = i; break;
				case 'b': if (++i < argc) bleed    = i; break;
			}
		}
	}

	if (output == -1)
	{
		std::cout << "Usage: {file-list} | texpack -o <output> " <<
			"[-p <padding>] [-m <metadata>] [-b]" <<
			std::endl;

		return 0;
	}

	pkr::Params params;

	params.output   = argv[output];
	params.metadata = metadata != -1 ? argv[metadata]      : 0;
	params.padding  = padding  != -1 ? atoi(argv[padding]) : 0;
	params.bleed    = bleed    != -1 ? atoi(argv[bleed])   : 0;

	pkr::pack(std::cin, params);

	return 0;
}
