#include <limits>
#include <cmath>
#include <cstdlib>
#include <algorithm>

#include "MaxRects.h"

namespace rbp {

static int common_interval_length(int i1start, int i1end, int i2start, int i2end)
{
	if (i1end < i2start || i2end < i1start)
		return 0;

	return std::min(i1end, i2end) - std::max(i1start, i2start);
}

static bool is_contained_in(const Rect &a, const Rect &b)
{
	return a.x >= b.x && a.y >= b.y
		&& a.x + a.width <= b.x + b.width
		&& a.y + a.height <= b.y + b.height;
}

MaxRects::MaxRects(int width, int height, bool rotate)
{
	width_ = width;
	height_ = height;
	rotate_ = rotate;

	Rect rect;

	rect.x = 0;
	rect.y = 0;
	rect.width = width;
	rect.height = height;

	free_.push_back(rect);
}

size_t MaxRects::insert(int mode, const std::vector<RectSize> &rects, std::vector<size_t> &rects_indices,
	std::vector<Rect> &result, std::vector<size_t> &result_indices)
{
	result.clear();
	result_indices.clear();

	result.reserve(rects_indices.size());
	result_indices.reserve(rects_indices.size());

	std::vector<size_t> &idx = rects_indices;

	while (idx.size() > 0)
	{
		Rect bestNode;

		int bestScore1 = std::numeric_limits<int>::max();
		int bestScore2 = std::numeric_limits<int>::max();
		int bestRectIndex = -1;

		for (size_t i = 0; i < idx.size(); ++i)
		{
			int score1;
			int score2;

			const RectSize &rect = rects[idx[i]];

			Rect newNode = score_rect(rect.width, rect.height, mode, score1, score2);

			if (score1 < bestScore1 || (score1 == bestScore1 && score2 < bestScore2))
			{
				bestScore1 = score1;
				bestScore2 = score2;
				bestNode = newNode;
				bestRectIndex = i;
			}
		}

		if (bestNode.height == 0 || bestRectIndex == -1)
			break;

		place_rect(bestNode);

		result.push_back(bestNode);
		result_indices.push_back(idx[bestRectIndex]);
		idx.erase(idx.begin() + bestRectIndex);
	}

	return result.size();
}

void MaxRects::place_rect(const Rect &node)
{
	size_t numRectanglesToProcess = free_.size();

	for (size_t i = 0; i < numRectanglesToProcess; ++i)
	{
		if (split_free_node(free_[i], node))
		{
			free_.erase(free_.begin() + i);
			--i;
			--numRectanglesToProcess;
		}
	}

	prune_free_list();
	used_.push_back(node);
}

Rect MaxRects::score_rect(int width, int height, int mode, int &score1, int &score2)
{
	Rect newNode;

	score1 = std::numeric_limits<int>::max();
	score2 = std::numeric_limits<int>::max();

	switch (mode)
	{
		case ShortSide:
			newNode = find_ss(width, height, score1, score2);
			break;

		case BottomLeft:
			newNode = find_bl(width, height, score1, score2);
			break;

		case ContactPoint:
			newNode = find_cp(width, height, score1);
			score1 = -score1;
			break;

		case LongSide:
			newNode = find_ls(width, height, score2, score1);
			break;

		case BestArea:
			newNode = find_ba(width, height, score1, score2);
			break;
	}

	if (newNode.height == 0)
	{
		score1 = std::numeric_limits<int>::max();
		score2 = std::numeric_limits<int>::max();
	}

	return newNode;
}

Rect MaxRects::find_bl(int width, int height, int &bestY, int &bestX)
{
	Rect bestNode = {0};

	bestY = std::numeric_limits<int>::max();

	for (size_t i = 0; i < free_.size(); ++i)
	{
		if (free_[i].width >= width && free_[i].height >= height)
		{
			int topSideY = free_[i].y + height;

			if (topSideY < bestY || (topSideY == bestY && free_[i].x < bestX))
			{
				bestNode.x = free_[i].x;
				bestNode.y = free_[i].y;
				bestNode.width = width;
				bestNode.height = height;
				bestY = topSideY;
				bestX = free_[i].x;
			}
		}

		if (rotate_ && free_[i].width >= height && free_[i].height >= width)
		{
			int topSideY = free_[i].y + width;

			if (topSideY < bestY || (topSideY == bestY && free_[i].x < bestX))
			{
				bestNode.x = free_[i].x;
				bestNode.y = free_[i].y;
				bestNode.width = height;
				bestNode.height = width;
				bestY = topSideY;
				bestX = free_[i].x;
			}
		}
	}

	return bestNode;
}

Rect MaxRects::find_ss(int width, int height, int &bestShortSideFit, int &bestLongSideFit)
{
	Rect bestNode = {0};

	bestShortSideFit = std::numeric_limits<int>::max();

	for (size_t i = 0; i < free_.size(); ++i)
	{
		if (free_[i].width >= width && free_[i].height >= height)
		{
			int leftoverHoriz = std::abs(free_[i].width - width);
			int leftoverVert = std::abs(free_[i].height - height);
			int shortSideFit = std::min(leftoverHoriz, leftoverVert);
			int longSideFit = std::max(leftoverHoriz, leftoverVert);

			if (shortSideFit < bestShortSideFit ||
				(shortSideFit == bestShortSideFit && longSideFit < bestLongSideFit))
			{
				bestNode.x = free_[i].x;
				bestNode.y = free_[i].y;
				bestNode.width = width;
				bestNode.height = height;
				bestShortSideFit = shortSideFit;
				bestLongSideFit = longSideFit;
			}
		}

		if (rotate_ && free_[i].width >= height && free_[i].height >= width)
		{
			int flippedLeftoverHoriz = std::abs(free_[i].width - height);
			int flippedLeftoverVert = std::abs(free_[i].height - width);
			int flippedShortSideFit = std::min(flippedLeftoverHoriz, flippedLeftoverVert);
			int flippedLongSideFit = std::max(flippedLeftoverHoriz, flippedLeftoverVert);

			if (flippedShortSideFit < bestShortSideFit ||
				(flippedShortSideFit == bestShortSideFit && flippedLongSideFit < bestLongSideFit))
			{
				bestNode.x = free_[i].x;
				bestNode.y = free_[i].y;
				bestNode.width = height;
				bestNode.height = width;
				bestShortSideFit = flippedShortSideFit;
				bestLongSideFit = flippedLongSideFit;
			}
		}
	}

	return bestNode;
}

Rect MaxRects::find_ls(int width, int height, int &bestShortSideFit, int &bestLongSideFit)
{
	Rect bestNode = {0};

	bestLongSideFit = std::numeric_limits<int>::max();

	for (size_t i = 0; i < free_.size(); ++i)
	{
		if (free_[i].width >= width && free_[i].height >= height)
		{
			int leftoverHoriz = std::abs(free_[i].width - width);
			int leftoverVert = std::abs(free_[i].height - height);
			int shortSideFit = std::min(leftoverHoriz, leftoverVert);
			int longSideFit = std::max(leftoverHoriz, leftoverVert);

			if (longSideFit < bestLongSideFit ||
				(longSideFit == bestLongSideFit && shortSideFit < bestShortSideFit))
			{
				bestNode.x = free_[i].x;
				bestNode.y = free_[i].y;
				bestNode.width = width;
				bestNode.height = height;
				bestShortSideFit = shortSideFit;
				bestLongSideFit = longSideFit;
			}
		}

		if (rotate_ && free_[i].width >= height && free_[i].height >= width)
		{
			int leftoverHoriz = std::abs(free_[i].width - height);
			int leftoverVert = std::abs(free_[i].height - width);
			int shortSideFit = std::min(leftoverHoriz, leftoverVert);
			int longSideFit = std::max(leftoverHoriz, leftoverVert);

			if (longSideFit < bestLongSideFit ||
				(longSideFit == bestLongSideFit && shortSideFit < bestShortSideFit))
			{
				bestNode.x = free_[i].x;
				bestNode.y = free_[i].y;
				bestNode.width = height;
				bestNode.height = width;
				bestShortSideFit = shortSideFit;
				bestLongSideFit = longSideFit;
			}
		}
	}

	return bestNode;
}

Rect MaxRects::find_ba(int width, int height, int &bestAreaFit, int &bestShortSideFit)
{
	Rect bestNode = {0};

	bestAreaFit = std::numeric_limits<int>::max();

	for (size_t i = 0; i < free_.size(); ++i)
	{
		int areaFit = free_[i].width * free_[i].height - width * height;

		if (free_[i].width >= width && free_[i].height >= height)
		{
			int leftoverHoriz = std::abs(free_[i].width - width);
			int leftoverVert = std::abs(free_[i].height - height);
			int shortSideFit = std::min(leftoverHoriz, leftoverVert);

			if (areaFit < bestAreaFit || (areaFit == bestAreaFit && shortSideFit < bestShortSideFit))
			{
				bestNode.x = free_[i].x;
				bestNode.y = free_[i].y;
				bestNode.width = width;
				bestNode.height = height;
				bestShortSideFit = shortSideFit;
				bestAreaFit = areaFit;
			}
		}

		if (rotate_ && free_[i].width >= height && free_[i].height >= width)
		{
			int leftoverHoriz = std::abs(free_[i].width - height);
			int leftoverVert = std::abs(free_[i].height - width);
			int shortSideFit = std::min(leftoverHoriz, leftoverVert);

			if (areaFit < bestAreaFit || (areaFit == bestAreaFit && shortSideFit < bestShortSideFit))
			{
				bestNode.x = free_[i].x;
				bestNode.y = free_[i].y;
				bestNode.width = height;
				bestNode.height = width;
				bestShortSideFit = shortSideFit;
				bestAreaFit = areaFit;
			}
		}
	}

	return bestNode;
}

int MaxRects::score_node_cp(int x, int y, int width, int height)
{
	int score = 0;

	if (x == 0 || x + width == width_)
		score += height;

	if (y == 0 || y + height == height_)
		score += width;

	for (size_t i = 0; i < used_.size(); ++i)
	{
		if (used_[i].x == x + width || used_[i].x + used_[i].width == x)
			score += common_interval_length(used_[i].y, used_[i].y + used_[i].height, y, y + height);

		if (used_[i].y == y + height || used_[i].y + used_[i].height == y)
			score += common_interval_length(used_[i].x, used_[i].x + used_[i].width, x, x + width);
	}

	return score;
}

Rect MaxRects::find_cp(int width, int height, int &bestContactScore)
{
	Rect bestNode = {0};

	bestContactScore = -1;

	for (size_t i = 0; i < free_.size(); ++i)
	{
		if (free_[i].width >= width && free_[i].height >= height)
		{
			int score = score_node_cp(free_[i].x, free_[i].y, width, height);

			if (score > bestContactScore)
			{
				bestNode.x = free_[i].x;
				bestNode.y = free_[i].y;
				bestNode.width = width;
				bestNode.height = height;
				bestContactScore = score;
			}
		}

		if (rotate_ && free_[i].width >= height && free_[i].height >= width)
		{
			int score = score_node_cp(free_[i].x, free_[i].y, height, width);

			if (score > bestContactScore)
			{
				bestNode.x = free_[i].x;
				bestNode.y = free_[i].y;
				bestNode.width = height;
				bestNode.height = width;
				bestContactScore = score;
			}
		}
	}

	return bestNode;
}

bool MaxRects::split_free_node(Rect freeNode, const Rect &usedNode)
{
	if (usedNode.x >= freeNode.x + freeNode.width || usedNode.x + usedNode.width <= freeNode.x ||
		usedNode.y >= freeNode.y + freeNode.height || usedNode.y + usedNode.height <= freeNode.y)
		return false;

	if (usedNode.x < freeNode.x + freeNode.width && usedNode.x + usedNode.width > freeNode.x)
	{
		if (usedNode.y > freeNode.y && usedNode.y < freeNode.y + freeNode.height)
		{
			Rect newNode = freeNode;
			newNode.height = usedNode.y - newNode.y;
			free_.push_back(newNode);
		}

		if (usedNode.y + usedNode.height < freeNode.y + freeNode.height)
		{
			Rect newNode = freeNode;
			newNode.y = usedNode.y + usedNode.height;
			newNode.height = freeNode.y + freeNode.height - (usedNode.y + usedNode.height);
			free_.push_back(newNode);
		}
	}

	if (usedNode.y < freeNode.y + freeNode.height && usedNode.y + usedNode.height > freeNode.y)
	{
		if (usedNode.x > freeNode.x && usedNode.x < freeNode.x + freeNode.width)
		{
			Rect newNode = freeNode;
			newNode.width = usedNode.x - newNode.x;
			free_.push_back(newNode);
		}

		if (usedNode.x + usedNode.width < freeNode.x + freeNode.width)
		{
			Rect newNode = freeNode;
			newNode.x = usedNode.x + usedNode.width;
			newNode.width = freeNode.x + freeNode.width - (usedNode.x + usedNode.width);
			free_.push_back(newNode);
		}
	}

	return true;
}

void MaxRects::prune_free_list()
{
	for (size_t i = 0; i < free_.size(); ++i)
	{
		for (size_t j = i + 1; j < free_.size(); ++j)
		{
			if (is_contained_in(free_[i], free_[j]))
			{
				free_.erase(free_.begin() + i);
				--i;
				break;
			}

			if (is_contained_in(free_[j], free_[i]))
			{
				free_.erase(free_.begin() + j);
				--j;
			}
		}
	}
}

}
