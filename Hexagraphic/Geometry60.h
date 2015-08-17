#pragma once

#include "Polygon.h"

class Geometry60
{

public:
	//static inline void rot_cw(Point& pt, int size)
	//{
	//	reflect30(pt, size);
	//	reflect45(pt, size);
	//}
	//static inline void rot_ccw(Point& pt, int size)
	//{
	//	reflect45(pt, size);
	//	reflect30(pt, size);
	//}

	static inline void rot_cw(Point& point, const Point& centre)
	{
		point -= centre;
		reflect30(point);
		reflect45(point);
		point += centre;
	}
	static inline void rot_ccw(Point& point, const Point& centre)
	{
		point -= centre;
		reflect45(point);
		reflect30(point);
		point += centre;
	}

	//static inline void reflect45(Point& pt, int size)
	//{
	//	int d = size * 2 - (pt.x + pt.y);
	//	pt.x += d;
	//	pt.y += d;
	//}
	//static inline void reflect30(Point& pt, int size)
	//{
	//	pt.y = 3 * size - (pt.x + pt.y);
	//}

	static inline void reflect45(Point& pt)
	{
		int d = -(pt.x + pt.y);
		pt.x += d;
		pt.y += d;
	}
	static inline void reflect30(Point& pt)
	{
		pt.y = -(pt.x + pt.y);
	}
};