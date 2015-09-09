#include "HexTile.h"
#include <algorithm>

using namespace hx;

bool hx::operator<(const Edge& lhs, const Edge& rhs)
{
	const unsigned sz = std::min(lhs.size(), rhs.size());
	for (unsigned i = 0; i < sz; ++i)
	{
		if (lhs[i] < rhs[i])
			return true;
		else if (rhs[i] < lhs[i])
			return false;
	}
	return lhs.size() < rhs.size();
}

bool hx::operator<(const HexPoly& lhs, const HexPoly& rhs)
{
	const unsigned sz = std::min(lhs.edges_.size(), rhs.edges_.size());
	for (unsigned i = 0; i < sz; ++i)
	{
		if (lhs.edges_[i] < rhs.edges_[i])
			return true;
		else if (rhs.edges_[i]< lhs.edges_[i])
			return false;
	}
	return lhs.edges_.size() < rhs.edges_.size();
}

bool hx::operator==(const HexPoly& lhs, const HexPoly& rhs)
{
	return !(lhs < rhs || rhs < lhs);
}

static Polygon create_hexagon(int n)
{
	using namespace boost::polygon::operators;
	using namespace boost::polygon;

	PolygonSet ps;
	Polygon p = {
		{ n , 0 },
		{ 0, n },
		{ -n, n },
		{ -n, 0 },
		{ 0, -n },
		{ n , -n } };
	assign(ps, p);
	for (Point& pt : ps.front())
		printf("(%d, %d)\n", pt.x,pt.y);
	return ps.front();
}

static HexPoly::VertexOffsets get_vertices(void)
{
	const int n = HexPoly::HEX_SIDE;
	HexPoly::VertexOffsets p;
	for (int y = -n, i = 0; y <= n; ++y)
	{
		for (int x = -n; x <= n; ++x, ++i)
			p[i] = Point(x, y);
	}
	return p;
}
static HexPoly::TransformTable get_rotation_table()
{
	cv::Matx<int, 2, 2>
		rT(0, 1, 1, 0),
		rD(0, -1, -1, 0),
		rS(1, 0, -1, -1),
		rI(1, 0, 0, 1);

	HexPoly::TransformTable r;
	using t = HexPoly::Transform;

	r[t::R0] = rI;
	r[t::R60] = rS*rD;
	r[t::R120] = rT*rS;
	r[t::R180] = rD*rT;
	r[t::R240] = rS*rT;
	r[t::R300] = rD*rS;

	r[t::T0] = rT * r[t::R0];
	r[t::T60] = rT * r[t::R60];
	r[t::T120] = rT * r[t::R120];
	r[t::T180] = rT * r[t::R180];
	r[t::T240] = rT * r[t::R240];
	r[t::T300] = rT * r[t::R300];

	for (int i = 0; i < t::MAX; ++i)
	{
		cv::Matx<int, 2, 2> mt = r[i];
		cv::flip(mt, mt, -1);
		for (int j = 0; j < t::MAX; ++j)
		{
			if (mt(0) == r[j](0) && mt(1) == r[j](1) && mt(2) == r[j](2) && mt(3) == r[j](3))
				printf("%d ~~> %d\n",i,j);

		}
	}

	return r;
}

HexPoly::VertexFlags  HexPoly::init_flags_(void)
{
	VertexFlags f;	
	memset(&f[0], 0, sizeof(f));

	printf("flags size : %d\n", sizeof(Flags));

	for (unsigned i = 0; i < points_.size(); ++i)
	{
		int d = points_[i].x + points_[i].y;
		f[i].is_tile = d*d <= HEX_SIDE*HEX_SIDE;
	}

	for (unsigned i = 0; i < primary_.size() - 1; ++i)
	{
		Point p = primary_[i];
		Point s = primary_[i+1]-p;
		s.x /= HEX_SIDE;
		s.y /= HEX_SIDE;		
		for (unsigned j = 0; j < HEX_SIDE; ++j, p += s)
		{
			int k = indexer(p);
			f[k].is_outline = true;
			f[k].edge_index = i;
		}			
	}


	for (int y = -HEX_SIDE, i = 0; y <= HEX_SIDE; ++y)
	{
		for (int x = -HEX_SIDE; x <= HEX_SIDE; ++x, ++i)
		{			
			if (!f[i].is_tile)
				printf(" ");
			else if (f[i].is_outline)
				printf("%d", f[i].edge_index);
			else
				printf(".");

		}
		printf("\n");
	}

	return f;
};


const Polygon HexPoly::primary_ = create_hexagon(HexPoly::HEX_SIDE);
const HexPoly::VertexOffsets HexPoly::points_ = get_vertices();
const HexPoly::TransformTable HexPoly::rot_lut_ = get_rotation_table();
const HexPoly::VertexFlags HexPoly::flags_ = init_flags_();