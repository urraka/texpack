#pragma once

#include <vector>
#include <cstddef>

namespace rbp
{
	struct RectSize
	{
		int width;
		int height;
	};

	struct Rect
	{
		int x;
		int y;
		int width;
		int height;
	};

	class MaxRects
	{
	public:
		MaxRects(int width, int height, bool rotate = true);

		enum Mode
		{
			ShortSide = 1,
			LongSide,
			BestArea,
			BottomLeft,
			ContactPoint
		};

		size_t insert(int mode, const std::vector<RectSize> &rects, std::vector<size_t> &rects_indices,
			std::vector<Rect> &result, std::vector<size_t> &result_indices);

	private:
		int width_;
		int height_;
		bool rotate_;

		std::vector<Rect> used_;
		std::vector<Rect> free_;

		Rect score_rect(int width, int height, int mode, int &score1, int &score2);
		void place_rect(const Rect &node);
		int score_node_cp(int x, int y, int width, int height);

		Rect find_bl(int width, int height, int &bestY, int &bestX);
		Rect find_ss(int width, int height, int &bestShortSideFit, int &bestLongSideFit);
		Rect find_ls(int width, int height, int &bestShortSideFit, int &bestLongSideFit);
		Rect find_ba(int width, int height, int &bestAreaFit, int &bestShortSideFit);
		Rect find_cp(int width, int height, int &contactScore);

		bool split_free_node(Rect freeNode, const Rect &usedNode);
		void prune_free_list();
	};
}
