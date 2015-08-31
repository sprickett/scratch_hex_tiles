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

	static std::vector<HexPoly> generate_tiles(const PolygonSet& polygons)
	{
		return std::vector<HexPoly>();
	}

	HexPoly(PolygonSet& polygons, const Point& offset)
	{
		using namespace boost::polygon::operators;
		using namespace boost::polygon;

		PolygonSet crop, fore, back;

		const int d = SIDE;

		// crop the polygons to a box twice the size of the tile;
		crop += rectangle_data<int>(offset.x - d, offset.y - d, offset.x + d, offset.y + d);
		crop &= polygons; 

		crop += polygons;


		// move the cropped poygons back by the offset
		for_each(crop.begin(), crop.end(), [&](Polygon& p)
		{ 
			move(p, HORIZONTAL, -offset.x);
			move(p, VERTICAL, -offset.y);
		});

		Polygon mask = primary_;
		scale_up(mask, 2);
		scale_up(crop, 2);
		
		fore += crop & mask;
		scale_down(fore, 2);
		scale_down(crop, 2);

		// find edges		
		//for (auto& poly : fore)
		//{
		//	Edge edge;
		//	bool is_edge = true;
		//	int i1 = indexer(poly.back());
		//	for (auto& fi = poly.begin()+1; fi < poly.end(); ++fi)
		//	{
		//		int i0 = i1;
		//		i1= indexer(*fi);
		//		if (!flags_[i0].is_outline || !flags_[i1].is_outline)
		//		{
		//			if (edge.empty())
		//		}

		//		if (!flags_[i].is_outline)
		//			edge.push_back(i);
		//		else if (fi != poly.begin() && !flags_[indexer(*(fi-1))].is_outline)
		//			edge.push_back(i);
		//		else if (fi + 1 != poly.end() && !flags_[indexer(*(fi + 1))].is_outline)
		//			edge.push_back(i);
		//		else for (auto& poly : crop)
		//		{
		//			if(std::find(poly.begin(), poly.end(), *fi) != poly.end())
		//			{
		//				edge.push_back(i);
		//				break;
		//			}		
		//		}

		//	}
		//	if (!edge.empty())
		//		edges_.push_back(edge);
		//	printf("edge %d\n", edge.size());
		//}

		edges_.emplace_back(0); // add new empty edge
		for (auto& poly : fore)
		{	
			int edge_index = edges_.size() - 1;
			for (auto& fi = poly.begin(); fi < poly.end(); ++fi)
			{
				Edge& e = edges_.back();
				int i = indexer(*fi);

				if (!flags_[i].is_outline)  
				{
					if (e.empty() && fi != poly.begin()) // start of edge
						e.emplace_back(indexer(*(fi - 1))); // if possible add start 
					e.emplace_back(i);
				}					
				else if (!e.empty()) // end of edge
				{
					e.push_back(i); // add end
					edges_.emplace_back(0); // add new empty edge
				}
			}
			// if an edge has been found add a new empty edge
			if (!edges_.back().empty())
			{
				// logic here for joining the first and last edges if necessary
				edges_.emplace_back(0);
			}
		}
		if (!edges_.back().empty())
			edges_.pop_back(); // 


		printf("edges %d -> ", edges_.size());
		for (auto& e : edges_)
			printf(" %d", e.size());
		printf("\n");

		back += fore ^ primary_;



		//assign(polygons,primary_);
		
		polygons = fore;
		for_each(polygons.begin(), polygons.end(), [&](Polygon& p)
		{
			move(p, HORIZONTAL, offset.x);
			move(p, VERTICAL, offset.y);
		});

	}

	const std::vector<Edge>& edges(){ return edges_; }

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
		int q = (x - y) % 3; // -2 -1 0 1 2

		q += (q < 0) * 3;
		x -= q & 1;
		y -= q == 2;
		x = (x - y) / 3;
		y += x - ((q == 0) & (xr + yr < HEX_SIDE));
		return Point(x, y);
	}
	static Point hexagon_centre(const Point& p)
	{
		int y = p.y - p.x + 1;
		int x = p.x * 3 + y;	
		return Point(x*HEX_SIDE, y*HEX_SIDE);
	}


	friend bool operator<(const HexPoly& lhs, const HexPoly& rhs);

private:
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