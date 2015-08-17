#pragma once

#include "Polygon.h"

class Geometry60
{

public:
	static inline void rot_cw(Point& pt, int size)
	{
		reflect30(pt, size);
		reflect45(pt, size);
	}
	static inline void rot_ccw(Point& pt, int size)
	{
		reflect45(pt, size);
		reflect30(pt, size);
	}

	static inline void rot_cw(Point& pt, const Point& centre)
	{
		pt -= centre;
		reflect30(pt);
		reflect45(pt);
		pt += centre;
	}
	static inline void rot_ccw(Point& pt, const Point& centre)
	{
		pt -= centre;
		reflect45(pt);
		reflect30(pt);
		pt += centre;
	}

	static inline void reflect45(Point& pt, int size)
	{
		int d = size * 2 - (pt.x + pt.y);
		pt.x += d;
		pt.y += d;
	}
	static inline void reflect30(Point& pt, int size)
	{
		pt.y = 3 * size - (pt.x + pt.y);
	}

	static inline void reflect45(Point& pt)
	{
		int d = -(pt.x + pt.y);
		pt.x += d;
		pt.y += d;
	}
	static inline void reflect30(Point& pt)
	{
		pt.y = -(pt.x+pt.y);
	}
};