#include "bleeding.h"

#include <vector>
#include <algorithm>

void bleed_apply(uint8_t *image, int width, int height, int iterations)
{
	const int N = width * height;

	std::vector<bool> A(N);
	std::vector<bool> B(N);

	for (int i = 0, j = 3; i < N; i++, j += 4)
		B[i] = (image[j] != 0);

	std::copy(B.begin(), B.end(), A.begin());

	int offsets[][2] = {
		{-1, -1},
		{ 0, -1},
		{ 1, -1},
		{-1,  0},
		{ 1,  0},
		{-1,  1},
		{ 0,  1},
		{ 1,  1}
	};

	for (int it = 0; it < iterations; it++)
	{
		size_t C = 0;

		for (int i = 0, j = 0, y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++, i += 4, j++)
			{
				if (!A[j])
				{
					int r = 0;
					int g = 0;
					int b = 0;
					int c = 0;

					for (size_t k = 0; k < sizeof(offsets) / sizeof(offsets[0]); k++)
					{
						int s = offsets[k][0];
						int t = offsets[k][1];

						if (x + s > 0 && x + s < width && y + t > 0 && y + t < height)
						{
							int index = i + 4 * s + 4 * t * width;

							if (A[j + s + t * width])
							{
								r += image[index + 0];
								g += image[index + 1];
								b += image[index + 2];

								c++;
							}
						}
					}

					if (c > 0)
					{
						image[i + 0] = r / c;
						image[i + 1] = g / c;
						image[i + 2] = b / c;

						B[j] = true;
					}

					C++;
				}
			}
		}

		if (C == 0)
			break;

		if (it + 1 < iterations)
			std::copy(B.begin(), B.end(), A.begin());
	}
}
