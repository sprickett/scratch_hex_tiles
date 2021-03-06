#pragma once

#include "Polygon.h"
#include "Geometry60.h"
#include <array>
#include <cstdint>


namespace hx
{
	typedef std::vector<int> Edge;
	typedef std::vector<Edge> EdgeSet;

	bool operator<(const Edge& lhs, const Edge& rhs);


class HexPoly
{
public:
	enum
	{
		HEX_SIDE = 12,
		SIDE = HEX_SIDE * 2 + 1,
		VERTEX_COUNT = SIDE*SIDE,
	};


	struct Flags
	{
		unsigned char is_tile : 1;
		unsigned char is_outline : 1;
		unsigned char edge_index : 3;

	};

	enum Transform
	{
		R0, R60, R120, R180, R240, R300,
		T0, T60, T120, T180, T240, T300,
		MAX
	};

	typedef std::array<Flags, VERTEX_COUNT> VertexFlags;
	typedef std::array<Point, VERTEX_COUNT> VertexOffsets;
	typedef std::array<cv::Matx<int, 2, 2>, Transform::MAX> TransformTable;

	template<class Op>
	static void hexagon_op(const boost::polygon::rectangle_data<int>& bounds, Op& op)
	{
		using namespace boost::polygon::operators;
		using namespace boost::polygon;

		const int x_end = xh(bounds) + HEX_SIDE;
		const int y_end = yh(bounds) + HEX_SIDE;

		std::array<Point, 4> offsets = {
			hexagon_centre(Point(-1, 0)),
			hexagon_centre(Point(0, -1)),
			hexagon_centre(Point(0, 0)),
			hexagon_centre(Point(1, -1)),
		};

		Point h0 = nearest_hexagon(Point(xl(bounds), yl(bounds)));
		for (Point col = hexagon_centre(h0); col.x < x_end; col.x += HEX_SIDE * 3)
		{
			for (auto& off : offsets)
			{
				for (Point h = col + off; h.y < y_end; h.y += HEX_SIDE * 3)
				{
					op(h);
				}
			}
		}
	}

	static unsigned inverse_transform(unsigned index)
	{
		return 0 < index & index < 6 ? (6 - index) : index;
	}


	static void debug_polyset(const PolygonSet& ps)
	{
		printf("%d polygons\n",ps.size());
		for (auto& poly : ps)
		{
			printf("%d\t", &poly - &ps[0]);
			for (auto& p : poly)
			{
				printf("(%d, %d) ", p.x, p.y);
			}
			printf("\n");
		}
	}


	static std::vector<HexPoly> generate_tiles2(const PolygonSet& polygons)
	{
		using namespace boost::polygon::operators;
		using namespace boost::polygon;

		std::vector<HexPoly> tiles;
		if (polygons.empty())
			return tiles;
		
		//printf("hello mum\n");


		// boost::polygon may remove edges with a length of 1 which is undesirable
		// to avoid this polygon data must be scaled up using safe scaling functions 
		// before any boost::polygon library operations 

		const int scale = 256;

		// copy and scale up polygons 
		PolygonSet polyset = polygons;
		scale_up_safe(polyset, scale); 

		// scale up the primary edge
		Polygon edge = primary_;
		scale_up(edge, scale); 

		// find the bounding box of the edge and bloat it by 1 and scale 
		rectangle_data<int> window;
		extents(window, primary_);
		bloat(window, 1);
		scale_up(window, scale);

		// find the bounding box of the scaled polygons and scale back down
		rectangle_data<int> bounds;
		extents(bounds, polyset);
		//bloat(bounds, scale / 2);
		scale_down(bounds, scale);

		PolygonSet fore, back, crop;

		// for every tile centre in the bounds...
		hexagon_op(bounds, [&](const Point& h) 
		{ 
			//   _____________
			//  |     |_____ /|
			//  |    /      | |<-- window
			//  |   /       | |
			//  |_ /        |_|
			//  | |        /  |
			//  | |       /<--|--- edge
			//  | |______/    |
			//  |/_______|____|
			
			rectangle_data<int> w = window;
			Point sh = h * scale; // find the scaled centre
			move(w, HORIZONTAL, sh.x); // move copy of window to the scaled tiles position
			move(w, VERTICAL, sh.y);
			assign(crop, w & polyset); // AND window with the polygons to find crop 
			translate(crop, -sh); // move the result back

			assign(fore, crop & edge); // AND with edge to find foreground

			scale_down_safe(fore, scale); // scale down to align points
			scale_up_safe(fore, scale); // scale back up to use library functions


			if (!fore.empty())
			{				
				assign(back, fore ^ edge); // XOR result with edge to find background (forces clean on foreground)			
				scale_down_safe(fore, scale); // scale down to final result
				scale_down_safe(back, scale);

				tiles.emplace_back(HexPoly());
				HexPoly& hp = tiles.back();

				hp.fore_ = fore;
				hp.back_ = back;

				translate(hp.fore_, h);
				translate(hp.back_, h);
			}

		});

		return tiles;
	}

	HexPoly(void)
	{}

	HexPoly(const PolygonSet& polygons, const Point& offset)
	{
		using namespace boost::polygon::operators;
		using namespace boost::polygon;

		PolygonSet crop;// , fore, back;

		const int d = SIDE;

		// crop the polygons to a box twice the size of the tile;
		//crop += rectangle_data<int>(offset.x - d, offset.y - d, offset.x + d, offset.y + d);
		//crop &= polygons; 

		crop += polygons;

		Polygon mask = primary_;
		move(mask, HORIZONTAL, offset.x);
		move(mask, VERTICAL, offset.y);

		scale_up(mask, 2);
		scale_up(crop, 2);
		assign(fore_, crop & mask);
		scale_down(fore_, 2);
		//scale_down(crop, 2);



		//edges_.emplace_back(0); // add new empty edge
		//for (auto& poly : fore_)
		//{	
		//	int edge_index = edges_.size() - 1;
		//	for (auto& fi = poly.begin(); fi < poly.end(); ++fi)
		//	{
		//		Edge& e = edges_.back();
		//		int i = indexer(*fi);

		//		if (!flags_[i].is_outline)  
		//		{
		//			if (e.empty() && fi != poly.begin()) // start of edge
		//				e.emplace_back(indexer(*(fi - 1))); // if possible add start 
		//			e.emplace_back(i);
		//		}					
		//		else if (!e.empty()) // end of edge
		//		{
		//			e.push_back(i); // add end
		//			edges_.emplace_back(0); // add new empty edge
		//		}
		//	}
		//	// if an edge has been found add a new empty edge
		//	if (!edges_.back().empty())
		//	{
		//		// logic here for joining the first and last edges if necessary
		//		edges_.emplace_back(0);
		//	}
		//}
		//if (!edges_.back().empty())
		//	edges_.pop_back(); // 


		//printf("edges %d -> ", edges_.size());
		//for (auto& e : edges_)
		//	printf(" %d", e.size());
		//printf("\n");






		assign(back_, fore_ ^ primary_);



		//assign(polygons,primary_);
		

	}

	

	size_t generate_edges(ushort* buf)
	{
		int sz = 0;
		for (auto e : edges_)
		{
			sz += (e.size() + 1) ;
		}
		
		if (buf)
		{
			unsigned i = 0;
			for (auto e : edges_)
			{				
				for (auto p : e)
				{
					buf[i++] = p;
				}
				buf[i++] = 0;	
			}	
			if (i != sz)
				printf("counts don't match!");
		}

		return sz * sizeof(ushort);
	}

	int minimise(void)
	{	
		int mn = VERTEX_COUNT;
		std::vector< std::pair<int, Edge* > > rots;

		// find orientations and edges with the minimum starting index  
		for (EdgeSet::iterator edge = edges_.begin(); edge != edges_.end(); ++edge)
		{
			Point pt = points_[edge->front()];
			Point bp = points_[edge->back()];
			for (int i = 0; i < Transform::MAX; ++i)
			{
				int j = indexer(rot_lut_[i] * pt);
				if (mn <= j)
				{
					if (mn < j) // new minimum
						rots.clear();
					rots.push_back({ i, &*edge });
				}			
			}
		}		

		int rot = rots.empty() ? 0 : rots.front().first;
		if (rots.size() > 1)  
		{ 
			// more than one edge and orientation have the minimum starting index
			const unsigned sz = rots.size();
			std::vector< Edge > edgs(sz);
			for (unsigned i = 0; i < sz; ++i)
			{
				edgs[i] = *rots[i].second; // copy the edge
				rotate_edge(edgs[i], rots[i].first); // rotate the copy
			}
			// find index of the minimum edge/rotation
			int index = distance(edgs.begin(), min_element(edgs.begin(), edgs.end()));
			rot = rots[index].first;
		}

		// rotate the edges
		for (EdgeSet::iterator edge = edges_.begin(); edge != edges_.end(); ++edge)
			rotate_edge(*edge, rot);

		// sort the edges
		sort(edges_.begin(), edges_.end());
	
		return rot;
	}

	
	

	static void rotate_edge(Edge& edge, int rot)
	{
		for (Edge::iterator pt = edge.begin(); pt != edge.end(); ++pt)
			*pt = indexer(rot_lut_[rot] * points_[*pt]);
	}
	static int indexer(const Point& p)
	{
		unsigned i = p.x + HEX_SIDE;
		unsigned j = p.y + HEX_SIDE;
		i *= i <= SIDE;
		j *= j <= SIDE;
		return i + j * SIDE;			
	}

	static Point nearest_hexagon(const Point& p)
	{
		int negx = p.x < 0;
		int negy = p.y < 0;

		int x = p.x / HEX_SIDE - negx;
		int y = p.y / HEX_SIDE - negy;
		int xr = p.x % HEX_SIDE + negx * HEX_SIDE;
		int yr = p.y % HEX_SIDE + negy * HEX_SIDE;
		int dy = xr + yr < HEX_SIDE;
		int q = (x - y) % 3; // -2 -1 0 1 2

		q += (q < 0) * 3;
		x -= q & 1;
		y -= q == 2;
		x = (x - y) / 3;
		y += x;
		y -= (q == 0) & dy;
		return Point(x, y);
	}
	static Point hexagon_centre(const Point& p)
	{
		int y = p.y - p.x + 1;
		int x = p.x * 3 + y;	
		return Point(x*HEX_SIDE, y*HEX_SIDE);
	}

	const std::vector<Edge>& edges() const { return edges_; }
	const PolygonSet& foreground() const { return fore_; }
	const PolygonSet& background() const { return back_; }
	friend bool operator<(const HexPoly& lhs, const HexPoly& rhs);

private:
	PolygonSet fore_;
	PolygonSet back_;
	std::vector<Edge> edges_;
	std::vector<int> mesh_;
	std::vector<int> mesh_inv_;

	static VertexFlags init_flags_(void);
	static const Polygon primary_;
	static const VertexOffsets points_;
	static const TransformTable rot_lut_;
	static const VertexFlags flags_;
	//friend class HexTileFactory;
};

	bool operator<(const HexPoly& lhs, const HexPoly& rhs);
	bool operator==(const HexPoly& lhs, const HexPoly& rhs);



}