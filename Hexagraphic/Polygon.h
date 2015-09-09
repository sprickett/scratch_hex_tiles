#pragma once

#include <boost/polygon/polygon.hpp>
#include <opencv2/core/core.hpp>
#include <boost/polygon/voronoi.hpp>

// the following is devired from the boost tutorial for registering custom types for use with the boost polygon library

typedef cv::Point2i Point;
typedef std::vector<Point> Polygon;
typedef std::vector<Polygon> PolygonSet;

namespace hx {	

	inline int sign(int n)
	{
		return (0 < n) - (n < 0);
	}


	inline void scale_up_safe(Point& point, int scale)
	{
		point.x *= scale;
		point.y *= scale;
	}
	inline void scale_up_safe(Polygon& polygon, int scale)
	{
		for (auto& p : polygon)
			scale_up_safe(p, scale);		
	}
	inline void scale_up_safe(PolygonSet& polyset, int scale)
	{
		for (auto& p : polyset)
			scale_up_safe(p, scale);
	}


	inline void scale_down_safe(int& interval, int scale)
	{
		int i = interval / scale;
		int r = (interval % scale) * 2;
		interval = i + (scale <= r) - (scale < -r);
	}
	inline void scale_down_safe(Point& point, int scale)
	{
		scale_down_safe(point.x, scale);
		scale_down_safe(point.y, scale);
	}
	inline void scale_down_safe(Polygon& polygon, int scale)
	{
		for (auto& p : polygon)
			scale_down_safe(p, scale);
	}
	inline void scale_down_safe(PolygonSet& polyset, int scale)
	{
		for (auto& p : polyset)
			scale_down_safe(p, scale);
	}

	

	inline void translate(Polygon& polygon, Point offset)
	{
		for (auto& p : polygon)
			p+=offset;
	}
	inline void translate(PolygonSet& polyset, Point offset)
	{
		for (auto& p : polyset)
			translate(p,offset);
	}

}


namespace boost { namespace polygon {

	template <>
	struct geometry_concept<Point> { typedef point_concept type; };
	template <>
	struct point_traits<Point> {
		typedef int coordinate_type;

		static inline coordinate_type get(const Point& point,
			orientation_2d orient) {
			if (orient == HORIZONTAL)
				return point.x;
			return point.y;
		}
	};

	template <>
	struct point_mutable_traits<Point> {
		typedef int coordinate_type;

		static inline void set(Point& point, orientation_2d orient, int value) {
			if (orient == HORIZONTAL)
				point.x = value;
			else
				point.y = value;
		}
		static inline Point construct(int x_value, int y_value) {
			Point retval;
			retval.x = x_value;
			retval.y = y_value;
			return retval;
		}
	};



	//we need to specialize our polygon concept mapping in boost polygon

	//first register CPolygon as a polygon_concept type
	template <>
	struct geometry_concept<Polygon>{ typedef polygon_concept type; };

	template <>
	struct polygon_traits<Polygon>
	{
		typedef int coordinate_type;
		typedef Polygon::const_iterator iterator_type;
		typedef Point point_type;

		// Get the begin iterator
		static inline iterator_type begin_points(const Polygon& t)
		{
			return t.begin();
		}

		// Get the end iterator
		static inline iterator_type end_points(const Polygon& t) 
		{
			return t.end();
		}

		// Get the number of sides of the polygon
		static inline std::size_t size(const Polygon& t) 
		{
			return t.size();
		}

		// Get the winding direction of the polygon
		static inline winding_direction winding(const Polygon& t) 
		{
			return unknown_winding;
		}
	};

	template <>
	struct polygon_mutable_traits<Polygon> 
	{
		//expects stl style iterators
		template <typename iT>
		static inline Polygon& set_points(Polygon& t, iT input_begin, iT input_end) 
		{
			t.clear();
			while (input_begin != input_end) 
			{
				t.push_back(Point());
				boost::polygon::assign(t.back(), *input_begin);
				++input_begin;
			}
			return t;
		}

	};


	//first we register CPolygonSet as a polygon set
	template <>
	struct geometry_concept<PolygonSet> { typedef polygon_set_concept type; };

	//next we map to the concept through traits
	template <>
	struct polygon_set_traits<PolygonSet> 
	{
		typedef int coordinate_type;
		typedef PolygonSet::const_iterator iterator_type;
		typedef PolygonSet operator_arg_type;

		static inline iterator_type begin(const PolygonSet& polygon_set) {
			return polygon_set.begin();
		}

		static inline iterator_type end(const PolygonSet& polygon_set) {
			return polygon_set.end();
		}

		//don't worry about these, just return false from them
		static inline bool clean(const PolygonSet& polygon_set) { return false; }
		static inline bool sorted(const PolygonSet& polygon_set) { return false; }
	};

	template <>
	struct polygon_set_mutable_traits<PolygonSet> 
	{
		template <typename input_iterator_type>
		static inline void set(PolygonSet& polygon_set, input_iterator_type input_begin, input_iterator_type input_end) 
		{
			polygon_set.clear();
			//this is kind of cheesy. I am copying the unknown input geometry
			//into my own polygon set and then calling get to populate the
			//deque
			polygon_set_data<int> ps;
			ps.insert(input_begin, input_end);
			ps.get(polygon_set);
			//if you had your own odd-ball polygon set you would probably have
			//to iterate through each polygon at this point and do something
			//extra
		}
	};
}}