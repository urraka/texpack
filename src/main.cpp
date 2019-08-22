#include "packer.h"
#include "help.h"
#include "getopt.h"

#include <cstdio>
#include <iostream>
#include <fstream>

void print_help()
{
	puts(help_text);
}

int main(int argc, char *argv[])
{
	pkr::Params params;

	struct option long_options[] = {
		{"help",           no_argument,       0, 'h'},
		{"alpha-bleeding", no_argument,       0, 'b'},
		{"premultiplied",  no_argument,       0, 'u'},
		{"POT",            no_argument,       0, 'P'},
		{"allow-rotate",   no_argument,       0, 'r'},
		{"pretty",         no_argument,       0, 'e'},
		{"trim",           no_argument,       0, 't'},
		{"max-size",       no_argument,       0, 'S'},
		{"indentation",    required_argument, 0, 'i'},
		{"output",         required_argument, 0, 'o'},
		{"metadata",       required_argument, 0, 'm'},
		{"padding",        required_argument, 0, 'p'},
		{"size",           required_argument, 0, 's'},
		{"mode",           required_argument, 0, 'M'},
		{"format",         required_argument, 0, 'f'},
		{0, 0, 0, 0}
	};

	while (true)
	{
		int option_index = 0;
		int code = getopt_long(argc, argv, "hbuPretSi:o:m:p:s:M:f:", long_options, &option_index);

		if (code == -1)
			break;

		switch (code)
		{
			case 'h':
				print_help();
				return 0;

			case 'b': params.bleed = true;         break;
			case 'u': params.premultiplied = true; break;
			case 'P': params.pot = true;           break;
			case 'r': params.rotate = true;        break;
			case 'e': params.pretty = true;        break;
			case 't': params.trim = true;          break;
			case 'o': params.output = optarg;      break;
			case 'm': params.metadata = optarg;    break;
			case 'M': params.mode = optarg;        break;
			case 'S': params.max_size = true;      break;
			case 'f': params.format = optarg;      break;

			case 'i':
				if (sscanf(optarg, "%d", &params.indentation) != 1)
				{
					fputs("Invalid value for indentation.\n", stderr);
					return 1;
				}
				break;

			case 'p':
				if (sscanf(optarg, "%d", &params.padding) != 1)
				{
					fputs("Invalid value for padding.\n", stderr);
					return 1;
				}
				break;

			case 's':
				if (sscanf(optarg, "%dx%d", &params.width, &params.height) != 2)
				{
					fputs("Invalid format for --size parameter.\n", stderr);
					return 1;
				}
				break;

			case 0:
			case '?':
			default:
				return 1;
		}
	}

	std::istream *input = &std::cin;
	std::ifstream file;

	if (optind < argc)
	{
		file.open(argv[optind]);

		if (!file.is_open())
		{
			fprintf(stderr, "Error reading file %s\n", argv[optind]);
			return 1;
		}

		input = &file;
	}

	return pkr::pack(*input, params);
}
