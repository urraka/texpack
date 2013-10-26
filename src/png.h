#pragma once

namespace png
{
	uint8_t *load(const char *path, int *width, int *height, int *channels);
	bool save(const char *filename, int width, int height, unsigned char *data);
}
