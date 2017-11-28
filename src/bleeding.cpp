#include "bleeding.h"
#include <cstring>
#include <vector>

void bleed_apply(uint8_t *image, int width, int height)
{
	const size_t N = width * height;

	std::vector<int8_t> opaque(N);
	std::vector<bool>   loose(N);
	std::vector<size_t> pending;
	std::vector<size_t> pendingNext;

	pending.reserve(N);
	pendingNext.reserve(N);

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

	for (size_t i = 0, j = 3; i < N; i++, j += 4)
	{
		if (image[j] == 0)
		{
			bool isLoose = true;

			int x = i % width;
			int y = i / width;

			for (int k = 0; k < 8; k++)
			{
				int s = offsets[k][0];
				int t = offsets[k][1];

				if (x + s >= 0 && x + s < width && y + t >= 0 && y + t < height)
				{
					size_t index = j + 4 * (s + t * width);

					if (image[index] != 0)
					{
						isLoose = false;
						break;
					}
				}
			}

			if (!isLoose)
				pending.push_back(i);
			else
				loose[i] = true;
		}
		else
		{
			opaque[i] = -1;
		}
	}

	while (pending.size() > 0)
	{
		pendingNext.clear();

		for (size_t p = 0; p < pending.size(); p++)
		{
			size_t i = pending[p] * 4;
			size_t j = pending[p];

			int x = j % width;
			int y = j / width;

			int r = 0;
			int g = 0;
			int b = 0;

			int count = 0;

			for (size_t k = 0; k < 8; k++)
			{
				int s = offsets[k][0];
				int t = offsets[k][1];

				if (x + s >= 0 && x + s < width && y + t >= 0 && y + t < height)
				{
					t *= width;

					if (opaque[j + s + t] & 1)
					{
						size_t index = i + 4 * (s + t);

						r += image[index + 0];
						g += image[index + 1];
						b += image[index + 2];

						count++;
					}
				}
			}

			if (count > 0)
			{
				image[i + 0] = r / count;
				image[i + 1] = g / count;
				image[i + 2] = b / count;

				opaque[j] = 0xFE;

				for (size_t k = 0; k < 8; k++)
				{
					int s = offsets[k][0];
					int t = offsets[k][1];

					if (x + s >= 0 && x + s < width && y + t >= 0 && y + t < height)
					{
						size_t index = j + s + t * width;

						if (loose[index])
						{
							pendingNext.push_back(index);
							loose[index] = false;
						}
					}
				}
			}
			else
			{
				pendingNext.push_back(j);
			}
		}

		if (pendingNext.size() > 0)
		{
			for (size_t p = 0; p < pending.size(); p++)
				opaque[pending[p]] >>= 1;
		}

		pending.swap(pendingNext);
	}
}
