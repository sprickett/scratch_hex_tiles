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
		reflect90(point);
		point += centre;
	}
	static inline void rot_ccw(Point& point, const Point& centre)
	{
		point -= centre;
		reflect90(point);
		reflect30(point);
		point += centre;
	}
	static inline void rot(Point& point, const Point& centre, int r60)
	{
		point -= centre;
		switch (r60 )
		{
		case 1:
			reflect30(point);
			reflect90(point);
			break;
		case 2:
			reflect0(point);
			reflect30(point);
			break;
		case 3:
			reflect90(point);
			reflect0(point);

			break;
		case 4:
			reflect30(point);
			reflect0(point);

			break;
		case 5:
			reflect90(point);
			reflect30(point);

			break;
		}
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

	static inline void reflect90(Point& pt)
	{
		int t = pt.x;
		pt.x = -pt.y;
		pt.y = -t;
	}
	static inline void reflect0(Point& pt)
	{
		int t = pt.x;
		pt.x = pt.y;
		pt.y = t;
	}
	static inline void reflect30(Point& pt)
	{
		pt.y = -(pt.x + pt.y);
	}
};