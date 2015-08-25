#pragma once

#include "Polygon.h"
#include "Geometry60.h"
#include <array>
#include <cstdint>


namespace hex
{
	

class HexPoly
{
public:
	enum
	{
		HEX_SIDE = 12,
		SIDE = HEX_SIDE * 2 + 1,
		VERTEX_COUNT = SIDE*SIDE,
		ORIENTATION_COUNT = 12,
	};

	typedef std::array<int, VERTEX_COUNT> VertexFlags;
	typedef std::array<Point, VERTEX_COUNT> VertexOffsets;
	typedef std::vector<int> Edge;
	typedef std::vector<Edge> EdgeVector;
	typedef std::array<cv::Matx<int, 2, 2>, ORIENTATION_COUNT> RotationTable;

	struct EdgeLessThan
	{
		bool operator()(const Edge& lhs, const Edge& rhs)
		{
			const unsigned sz = min(lhs.size(), rhs.size());
			for (unsigned i = 0; i < sz; ++i)
			{
				if (lhs[i] != rhs[i])
					return lhs[i] < rhs[i];
			}
			return lhs.size() < rhs.size();
		}
		bool operator()(const std::pair<int,Edge*>& lhs, const std::pair<int,Edge*>& rhs)
		{
			return this->operator()(*lhs.second, *rhs.second);
		}
	};
	//struct EdgeEqual
	//{
	//	bool operator()(const Edge& lhs, const Edge& rhs)
	//	{
	//		EdgeLessThan op;
	//		return !(op(lhs, rhs) || op(rhs, lhs));
	//	}
	//};
	struct LessThan
	{
		EdgeLessThan op;
		bool operator()(const HexPoly& lhs, const HexPoly& rhs)
		{
			const unsigned sz = min(lhs.edges_.size(), rhs.edges_.size());
			for (unsigned i = 0; i < sz; ++i)
			{
				if (op(lhs.edges_[i], rhs.edges_[i]))
					return true;
				else if (op(rhs.edges_[i], lhs.edges_[i]))
					return false;
			}
			return lhs.edges_.size() < rhs.edges_.size();
		}
	};





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
		vector< std::pair<int, Edge* > > rots;


		// find orientations and edges with the minimum starting index  
		for (EdgeVector::iterator edge = edges_.begin(); edge != edges_.end(); ++edge)
		{
			Point pt = points_[edge->front()];
			Point bp = points_[edge->back()];
			for (int i = 0; i < ORIENTATION_COUNT; ++i)
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

		if (rots.size()>1) // more than one edge and orientation have the minimum starting index  
		{
			const unsigned sz = rots.size();
			vector< Edge > edgs(sz);
			for (unsigned i = 0; i < sz; ++i)
			{
				edgs[i] = *rots[i].second; // copy the edge
				rotate_edge(edgs[i], rots[i].first); // rotate the copy
				rots[i].second = &edgs[i]; // set edge pointer to the address of the copy
			}
			//sort(rots.begin(), rots.end(), EdgeLessThan()); // sort the edge copies			
		}
		// the edge pointers can no longer safely be dereferenced! 

		int rot = 0;
		if (!rots.empty())
		{
			rot = rots.front().first;
			for (EdgeVector::iterator edge = edges_.begin(); edge != edges_.end(); ++edge)
				rotate_edge(*edge, rot);
			sort(edges_.begin(), edges_.end(), EdgeLessThan());
		}
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
		return (i + j * SIDE) * ((i|j)<=SIDE) ;			
	}
	//static Point offset(unsigned i)
	//{
	//	return Point(i % SIDE - HEX_SIDE, i / SIDE - HEX_SIDE);
	//}

private:
	std::vector<Edge> edges_;
	std::vector<int> mesh_;
	std::vector<int> mesh_inv_;

		
	static const VertexFlags flags_;
	static const VertexOffsets points_;
	static const RotationTable rot_lut_;
	friend class HexTileFactory;
};

class HexTileFactory
{
	Polygon primary_edge_;
	
};


class TileSet
{
public:
	std::vector<HexPoly> tiles;
};


class HexBlock
{
	enum
	{
		SHIFT = 8,
		SIDE = 1u << SHIFT,
		SIZE = SIDE*SIDE,
	};

	typedef std::array<int, SIZE > tiles_;


};


//cpp
static HexPoly::VertexFlags get_flags(void)
{
	HexPoly::VertexFlags f;
	return f;
};

const HexPoly::VertexFlags HexPoly::flags_ = get_flags();

static HexPoly::RotationTable get_rotation_table()
{
	HexPoly::RotationTable r;
	cv::Matx<int, 2, 2> 
		rT(0, 1, 1, 0),
		rD(0, -1, -1, 0),
		rS(1, 0, -1, -1),
		rI(1, 0, 0, 1);

	r[0] = rI;
	r[1] = rT*rS;
	r[2] = rS*rD;
	r[3] = rD*rD;
	r[4] = rD*rS;
	r[5] = rS*rT;
	for (int i = 0; i < 6; ++i)
		r[i+6] = rT*r[i];
	return r;

		//Mat22 rm = ; // ccw 1
		//Mat22 rm = rS*rD; // ccw 2
		//Mat22 rm = rD*rD; // cc/ccw 3

	//Mat22 rm = rT*rT; // cc/ccw 3
	//Mat22 rm = rS*rT; // cc 1
	//Mat22 rm = rD*rS; // ccw 2
}



const HexPoly::RotationTable HexPoly::rot_lut_ = get_rotation_table();

}