#include "packer.h"

#include <iostream>
#include <cstring>
#include <cstdlib>

int main(int argc, char *argv[])
{
	int output = -1;
	int padding = -1;

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-p") == 0)
		{
			if (++i < argc)
				padding = i;
		}
		else if (strcmp(argv[i], "-o") == 0)
		{
			if (++i < argc)
				output = i;
		}
	}

	if (output == -1)
	{
		std::cout << "Usage: {file-list} | texpack -o <name> [-p <padding>]" << std::endl;
		return 0;
	}

	padding = padding != -1 ? atoi(argv[padding]) : 0;

	pkr::pack(std::cin, argv[output], padding);

	return 0;
}
