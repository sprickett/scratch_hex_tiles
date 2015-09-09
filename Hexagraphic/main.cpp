
#include <opencv2\core\core.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <iostream>
using namespace std;

#include "Polygon.h"
#include "Geometry60.h"
#include "HexTile.h"


//struct Point :public Point
//{
//	Point(const Point& p = Point())
//	:Point(p){};
//};
//struct Point :public Point
//{
//	Point(const Point& p = Point())
//	:Point(p){};
//};
void filter(Polygon& poly)
{
	if (poly.empty())
		return;

	const unsigned sz = poly.size();
	Polygon filt(1,poly.front());
	filt.reserve(sz);
	unsigned i;

	for (i = 0; i < sz - 2; ++i)
	{
		Point v = poly[i + 1] - poly[i];
		if (v.cross(poly[i + 2] - poly[i + 1]))
			filt.push_back(poly[i + 1]);
	}
	filt.push_back(poly[i + 1]);
	swap(poly, filt);	
}

void filter(PolygonSet& ps)
{
	for (auto& poly : ps)
		filter(poly);
}

class Mapping
{
public:
	Mapping(int size)
		:size_(size)
	{}

	Point to_v_triangle(const Point& screen) const
	{
		return to_v_triangle(screen, size_);
	}
	Point to_h_triangle(const Point& screen) const
	{
		return to_h_triangle(screen, size_);
	}
	Point v_tri_to_screen(const Point& triangle) const
	{
		return v_tri_to_screen(triangle,size_);
	}
	Point h_tri_to_screen(const Point& triangle) const
	{
		return h_tri_to_screen(triangle,size_);
	}
	//template<class Typ>
	//Point to_screen(const Typ& index) const
	//{
	//	return to_screen(index, size_);
	//}




	static Point to_v_triangle(const Point& screen, int size)
	{
		Point t;
		screen2VTri_(t.x, t.y, screen.x, screen.y, size);
		return t;
	}
	static Point to_h_triangle(const Point& screen, int size)
	{
		Point t;
		screen2HTri_(t.x, t.y, screen.x, screen.y, size);
		return t;
	}
	static Point v_tri_to_screen(const Point& triangle, int size)
	{
		Point p;
		vTri2Screen_(p.x, p.y, triangle.x, triangle.y, size);
		return p;
	}
	static Point h_tri_to_screen(const Point& triangle, int size)
	{
		Point p;
		hTri2Screen_(p.x, p.y, triangle.x, triangle.y, size);
		return p;
	}

private:
	enum
	{
		Rt3NUM = 3691, Rt3DOM = 2131, //1.27137e-007
	};
	static inline int sign(int n)
	{
		return (0 < n) - (n < 0);
	}
	static inline void screen2VTri_(int& c, int& r, int x, int y, int size)
	{
		c = (Rt3DOM * (x + size)) / (Rt3NUM * size);
		r = y - size * c;
		r += size* sign(r); // offset for rounding
		r /= (size * 2);
	}
	static inline void screen2HTri_(int& c, int& r, int x, int y, int size)
	{
		r = (y + size) / (size*2) ;
		x *= Rt3NUM;
		c = Rt3DOM * 2 * size;
		x -= (r * c) ;
		x += c * sign(x);
		c = x / (c * 2) ;
	}
	static inline void vTri2Screen_(int& x, int& y, int c, int r, int size)
	{
		x = (size * c * Rt3NUM) / Rt3DOM;
		y = size * (2 * r + c);
	}
	static inline void hTri2Screen_(int& x, int& y, int c, int r, int size)
	{ 
		size *= 2;
		y = size * r;
		x = (size * Rt3DOM * (2 * c +  r)) / Rt3NUM;
	}


	int size_;
};





class MapperH
{
public:
	MapperH(int step = 3)
		: image_(900, 1600, CV_8UC3)
		, tri_(step)
		, hex_(step * 18)
		, cols_(tri_.to_v_triangle(Point(image_.cols,0)).x)
		, rows_(tri_.to_v_triangle(Point(0, image_.rows)).y)
	{
		
	}
	


	Point screen2tri(cv::Point pixel) const
	{
		return tri_.to_v_triangle(pixel);		
	}
	Point tri2screen(const Point& t) const
	{ 
		return tri_.v_tri_to_screen(t);
	}

	Point screen2hex(cv::Point pixel) const
	{
		return hex_.to_h_triangle(pixel);
	}
	Point hex2screen(const Point& t) const
	{
		return hex_.h_tri_to_screen(t);
	}


	void clear(const cv::Scalar& colour = cv::Scalar())
	{
		image_.setTo(colour);
	}



	void draw_point(const cv::Point& p, const cv::Scalar& colour)
	{
		image_.at<cv::Vec3b>(p) = cv::Vec3b(128, 128, 128);
	}
	void draw_line(const cv::Point& p0, const cv::Point& p1, const cv::Scalar& colour, int thickness = 1)
	{
		cv::line(image_, p0, p1, colour, thickness, CV_AA);
	}
	void draw_circle(const cv::Point& p0, int radius, const cv::Scalar& colour, int thickness = 1)
	{
		cv::circle(image_, p0, radius, colour, thickness, CV_AA);
	}
	void draw_text(const std::string& text, const cv::Point& p, double scale, const cv::Scalar& colour, int thickness = 1)
	{
		cv::putText(image_, text, p, cv::FONT_HERSHEY_SIMPLEX, scale, colour, thickness, CV_AA, false);
	}

	struct dot_operator
	{	
		dot_operator(const cv::Scalar& colour)
			:colour(colour[0], colour[1], colour[2])
		{}

		void operator()(cv::Mat& image, const Point& pt)
		{
			if ((unsigned)pt.x < image.cols && (unsigned)pt.y < image.rows)
				image.at<cv::Vec3b>(pt) = colour;
		}
		
		cv::Vec3b colour;
	};
	struct circle_operator
	{
		circle_operator(int radius, const cv::Scalar& colour, int thickness=1)
		:radius(radius)
		, thickness(thickness)
		,colour(colour[0], colour[1], colour[2])
		{}

		void operator()(cv::Mat& image, const Point& pt)
		{
			cv::circle(image, pt, radius, colour, thickness, CV_AA);
		}

		int radius;
		int thickness;
		cv::Scalar colour;
	};


	//void draw_dots_(const cv::Scalar& colour)
	//{
	//	cv::Vec3b bgr(colour[0], colour[1], colour[2]);
	//	Point t= Point(0,0);
	//	for (t.x = 0; t.x<cols_; ++t.x)
	//	{
	//		t.y = -t.x / 2;
	//		int ry = rows_+ t.y;
	//		for (; t.y<ry; ++t.y)
	//		{
	//			cv::Point p = tri_.to_screen(t);
	//			if ((unsigned)p.x < image_.cols && (unsigned)p.y < image_.rows)
	//				image_.at<cv::Vec3b>(p) = bgr;
	//		}
	//	}


	//}
	template <class DrawOp>
	void draw_tri_offset(DrawOp& drawop)
	{
		Point t = Point(0, 0);
		for (t.x = 0; t.x<cols_; ++t.x)
		{
			t.y = -t.x / 2;
			for (int ey = rows_ + t.y; t.y<ey; ++t.y)
			{
				drawop(image_,tri_.v_tri_to_screen(t));
			}
		}
	}

	template <class DrawOp>
	void draw_hex_offset(DrawOp& drawop)
	{
		Point t = Point(0, 0);
		for (t.y = 0; t.y<10; ++t.y)
		{
			t.x = -t.y / 2;
			for (int ex=10+t.x; t.x<ex; ++t.x)
			{
				drawop(image_, hex_.h_tri_to_screen(t));
			}
		}
	}



	void draw_hex_grid(int size, int ox, int oy, const cv::Scalar& colour = cv::Scalar(255, 255, 255))
	{
		int sml = size / 2;
		int big = (size + 1) / 2;
		//vector<Point> line = {
		//	Point(size * 2 - 1, 1),
		//	Point(size, 1),
		//	Point(1, size),
		//	Point(1, size * 2-1),
		//	Point(size, size*2-1),
		//	Point(size*2-1, size),
		//	Point(size * 2-1, 1),
		//};
		vector<Point> line = {
			Point(size * 2 , 0),
			Point(size, 0),
			Point(0, size),
			Point(0, size * 2),
			Point(size, size * 2),
			Point(size * 2, size),
			Point(size * 2, 0),
		};

		repeat(line, Point(size*2, -size), colour);

		//repeat_line(size, 0, size, 0, size, colour);
		//repeat_line(size, 0, -size, size, size, colour);
		//repeat_line(0, size, 0, size, size, colour);
		//repeat_line(size*2, 0, 0, size, size, colour);
		
		
		//repeat_line(0, size*2, size, 0, size, colour);
		//repeat_line(0, size, , size, size, colour);
		//repeat_line(0, size, size, -size, size, colour);

		//repeat_line(ox + big, oy + 0, size, 0, size, colour);
		//repeat_line(ox + size * 2, oy + size, size, 0, size, colour);
		//repeat_line(ox + big, oy + 0, -big, size, size, colour);
		//repeat_line(ox + size * 2, oy + size, -sml, size, size, colour);
		//repeat_line(ox + 0, oy + size, big, size, size, colour);
		//repeat_line(ox + size + big, oy + 0, sml, size, size, colour);
	}

	//void draw_hex_grid(int size, int ox, int oy, const cv::Scalar& colour = cv::Scalar(255, 255, 255))
	//{
	//	int sml = size / 2;
	//	int big = (size + 1) / 2;

	//	repeat_line(0, 1, sml, 0, size, colour);
	//	//repeat_line(ox + size * 2, oy + size, size, 0, size, colour);
	//	//repeat_line(ox + big, oy + 0, -big, size, size, colour);
	//	//repeat_line(ox + size * 2, oy + size, -sml, size, size, colour);
	//	//repeat_line(ox + 0, oy + size, big, size, size, colour);
	//	//repeat_line(ox + size + big, oy + 0, sml, size, size, colour);
	//}



	int cols(void)
	{
		return cols_;
	}
	int rows(void)
	{
		return rows_;
	}
	cv::Mat image(void)
	{
		return image_;
	}


private:
	void repeat_line(int ox, int oy, int vx, int vy, int size, const cv::Scalar& colour)
	{
		int xe = std::min(cols_, cols_ - vx);
		int ye = std::min(rows_, rows_ - vy);
		Point p, v;
		v.x = vx;
		v.y =  vy;
		for (int x = ox; x < xe; x += size * 2)		
		{
			p.x = x;
			for (int y = -x; y< ye-x; y += size*2 )
			{
				p.y = y;
				draw_line(tri2screen(p), tri2screen(p+v), colour);
			
			}
			//break;
		}
	}
	void repeat(const vector<Point>& line, Point step, const cv::Scalar& colour)
	{
		{
		
			Point rstep;// = step;
			rstep.x = step.y;
			rstep.y = step.x;//(step.x, step.y);
			for (int i = 1; i < line.size();++i)
			{
				Point p0 = line[i-1];
				Point p1 = line[i];

				for (int y = 0; y < 10; ++y)
				{
					for (int x = 0; x < 10; ++x)
						draw_line(tri2screen(p0+step*x), tri2screen(p1+step*x), colour);

					p0 += rstep;
					p1 += rstep;
					
				}

			}
		}
	}

	std::string name_;
	cv::Mat image_;
	Mapping tri_;
	Mapping hex_;
	int cols_;
	int rows_;
};

class handler
{
public:
	virtual bool mouse_move(int event, int x, int y, int flags);	
};




//class lines :public handler
//{
//	lines(void) :pline_(0){}
//	bool mouse_move(const mapper* map, int event, int x, int y, int flags)
//	{
//		switch (event)
//		{
//		case cv::EVENT_LBUTTONUP:
//			if (pline_ != lines_.end())
//			{
//				int i = map->get_index(x, y);
//				pline_->push_back(i);
//			}
//			break;
//		case cv::EVENT_LBUTTONDOWN:
//			if (pline_ == lines_.end())
//			{
//				lines_.push_back(vector<int>());
//				pline_ = lines_.begin() + lines_.size() - 1;
//			}
//			pline_->push_back(map->get_index(x, y));
//			break;
//		case cv::EVENT_RBUTTONDOWN:
//			if (pline_ != lines_.end() && pline_->size() < 2)
//			{
//				lines_.erase(pline_);
//			}
//			pline_ = lines_.end();
//			break;
//		}
//	}
////	//void draw_lines(const mapper* map)
////	//{
////	//	for (int i = 0; i < lines_.size(); ++i)
////	//	{
////	//		vector<int>& line = lines_[i];
////	//		cv::Point p0, p1 = map->get_point(line[0]);
////	//		for (int i = 1; i < line.size(); ++i)
////	//		{
////	//			p0 = p1;
////	//			p1 = map->get_point(line[i]);
////	//			cv::line(map->image_, p0, p1, cv::Scalar(255, 0, 255), 1, CV_AA);
////	//		}
////	//	}
////	//	cv::Point mos = map->get_point(map->get_index(mosx_, mosy_));
////	//	if (pline_ != lines_.end())
////	//		cv::line(image_, get_point(pline->back()), mos, cv::Scalar(255, 0, 255), 1, CV_AA);
////	//	cv::circle(image_, mos, 4, cv::Scalar(255, 0, 255), 1, CV_AA);
////	//}
////
//	std::vector< std::vector< int > > lines_;
//	std::vector< std::vector< int > >::iterator pline_;
//};

class mapped_image
{
public:
	mapped_image(const std::string& name = "alpha")
		:name_(name)
		, scene_()
		, draw_(false)
		, pline(0)
	{
		cv::namedWindow(name_);
		cv::setMouseCallback(name_, mouse_move_callback, this);
	}


	void draw(void)
	{
		scene_.clear();
		scene_.draw_hex_grid(12, 0, 0, cv::Scalar(128, 0, 0));
		scene_.draw_tri_offset(MapperH::dot_operator(cv::Scalar(255, 0,0)));
		scene_.draw_hex_offset(MapperH::circle_operator(4,cv::Scalar(255, 0, 0)));
		//scene_.draw_hex_grid(3,1,0,cv::Scalar(128, 0, 0));
		

		draw_lines();
	}

	int show()
	{
		
		cv::imshow(name_, scene_.image());
		int key = cv::waitKey(33);
		return key;
	}

	void mouse_move(int event, int x, int y, int flags)
	{	
		

		mos_.x = x;
		mos_.y = y;

		//EVENT_FLAG_LBUTTON
		//EVENT_FLAG_RBUTTON
		//EVENT_FLAG_MBUTTON
		//EVENT_FLAG_CTRLKEY
		//EVENT_FLAG_SHIFTKEY
		//EVENT_FLAG_ALTKEY
		
		
		if (flags & cv::EVENT_FLAG_CTRLKEY)
		{
			typedef cv::Matx<int, 2, 2> Mat22;
			Mat22 rD(0, -1, -1, 0);
			Mat22 rS(1, 0, -1, -1);
			Mat22 rT(0, 1, 1, 0);
			Mat22 rI(1, 0, 0, 1);

			

			//Mat22 rm = rT*rS; // ccw 1
			//Mat22 rm = rS*rD; // ccw 2
			Mat22 rm = rD*rD; // cc/ccw 3

			//Mat22 rm = rT*rT; // cc/ccw 3
			//Mat22 rm = rS*rT; // cc 1
			//Mat22 rm = rD*rS; // ccw 2




			Point centre = scene_.screen2tri(scene_.hex2screen(scene_.screen2hex(mos_)));
			switch (event)
			{
			case cv::EVENT_LBUTTONDOWN:
				for (int j = 0; j < polyset_.size(); ++j)
				{
					for (int i = 0; i < polyset_[j].size(); ++i)
					{
						polyset_[j][i] = centre + rm * (centre - polyset_[j][i]);
						//point = Point(point.y,point.x);
						//Geometry60::rot(point, centre, 4);
					}
					
				}
				
				break;
			case cv::EVENT_RBUTTONDOWN:
				for (auto& poly : polyset_)
				for (auto& point : poly)
					Geometry60::rot_ccw(point, centre);
				break;
			}
		}
		else
		{
			switch (event)
			{
			case cv::EVENT_LBUTTONUP:
				if (pline != 0)
				{
					pline->back() = scene_.screen2tri(mos_);		
				}
				break;
			case cv::EVENT_LBUTTONDOWN:
				if (pline == 0)
				{
					polyset_.push_back(vector<Point>());
					pline = &polyset_.back();
				}
				pline->push_back(scene_.screen2tri(mos_));
				break;
			case cv::EVENT_RBUTTONDOWN:
				if (pline != 0 && pline->size() < 2)
				{
					polyset_.erase(polyset_.begin() + (pline - &polyset_.front()));
				}
				pline = 0;
				break;
			}
		}





		//printf("%d %d %d %d\n", event, x, y, flags);
	}


	void voronoi()
	{
		using namespace boost::polygon::operators;
		using namespace boost::polygon;
		using boost::polygon::voronoi_builder;
		using boost::polygon::voronoi_diagram;

		if (polyset_.empty())
			return;

		pline = 0;



		rectangle_data<int> rect;
		extents(rect, polyset_);

		auto tiles= hx::HexPoly::generate_tiles2(polyset_);

		rectangle_data<int> rect2 = rect;
		bloat(rect2, 1);
		bloat(rect, 2);

		polyset_.clear();
		//polyset_ += rect2^rect;
		for (auto& hp : tiles)
			for (auto& p : hp.foreground())
				polyset_.push_back(p);
		
		
		//v_diagram.clear();
		//construct_voronoi(points.begin(), points.end(), &v_diagram);


	
		

	}

private:
	void draw_lines(void)
	{
		const cv::Scalar line_colour = cv::Scalar(255, 0, 255);
		const cv::Scalar text_colour = cv::Scalar(255, 255, 255);


		typedef boost::polygon::voronoi_diagram<double>::const_cell_iterator cellit;
		typedef boost::polygon::voronoi_diagram<double>::const_edge_iterator edgeit;

		int j = 0;
		for (cellit it = v_diagram.cells().begin(); it != v_diagram.cells().end(); ++it)
		{
			int i = it->source_index();
			Point p(scene_.tri2screen(polyset_.front()[i]));
			//scene_.draw_circle(p, (i+1)*2, cv::Scalar(64, 64, 255));
			//for (edgeit eit = it->->edges().begin(); it != v_diagram.edges().end(); ++it)
			//{

			//}

		}

		
		for (edgeit it = v_diagram.edges().begin(); it != v_diagram.edges().end(); ++it)
		{
			const auto& v0 = it->vertex0();
			const auto& v1 = it->vertex1();

			cv::Scalar colour(255, 255, 255);
			if (it->is_primary())
				colour = cv::Scalar(0, 255, 255);
			else if (it->is_secondary())
				colour = cv::Scalar(255, 255, 0);

			if (it->is_infinite())
				colour = cv::Scalar(0, 0, 255);
			else if (it->is_finite())
				colour = cv::Scalar(0, 255, 0);

			const auto& c = it->cell();
	
			int i0 = it->cell()->source_index();
			int i1 = it->twin()->cell()->source_index();
			Point p0(scene_.tri2screen(polyset_.front()[i0]));
			Point p1(scene_.tri2screen(polyset_.front()[i1]));
			if (i0<i1)
				scene_.draw_line(p0, p1, cv::Scalar(64, 64, 64));

		}


		//cv::Mat image = scene_.image();
		for (int i = 0; i < polyset_.size(); ++i)
		{
			vector<Point>& line = polyset_[i];
			cv::Point p0, p1 = scene_.tri2screen(line[0]);
			for (int i = 1; i < line.size(); ++i)
			{
				p0 = p1;
				p1 = scene_.tri2screen(line[i]);
				scene_.draw_line(p0, p1, cv::Scalar(255, 0, 255), 1);
			}
		}


		Point itri = scene_.screen2tri(mos_);

		Point ihex = hx::HexPoly::nearest_hexagon(itri);

		//Point ihex = scene_.screen2hex(mos_);

		Point tri = scene_.tri2screen(itri);
		Point hex = scene_.tri2screen(hx::HexPoly::hexagon_centre(ihex));

		
		


		if (pline != 0)
			scene_.draw_line( scene_.tri2screen(pline->back()), tri, line_colour);
		scene_.draw_circle(tri, 5, cv::Scalar(255, 0, 255));
		scene_.draw_circle(hex, 12, line_colour);
		//scene_.draw_circle( , 12, line_colour);

		stringstream ss;
		ss << (int)itri.x << ", " << (int)itri.y << "(" <<  (int)ihex.x << ", " << (int)ihex.y <<")";
		scene_.draw_text(ss.str(), tri, 0.5, text_colour);

		//ss = stringstream();
		//ss << (int)ihex.x << ", " << (int)ihex.y;
		//scene_.draw_text(ss.str(), hex, 0.5, text_colour);
	}



	static void mouse_move_callback(int event, int x, int y, int flags, void* userdata)
	{
		reinterpret_cast<mapped_image*>(userdata)->mouse_move(event, x, y, flags);
	}

	std::string name_;
	MapperH scene_;
	

	cv::Point mos_;
	bool draw_;
	handler* handler_;

	public:
	PolygonSet polyset_;
	boost::polygon::voronoi_diagram<double> v_diagram;
	Polygon*  pline;
	//PolygonSet polygons_;

	private:

	
	 




};






void test_polygon_set() 
{
	using namespace boost::polygon::operators;
	using namespace boost::polygon;
	PolygonSet ps1;
	ps1 += rectangle_data<int>(0, 0, 100, 100);
	printf("rect -> %d points\n", ps1.front().size());
	PolygonSet ps2;
	ps2 += rectangle_data<int>(50, 50, 150, 150);
	PolygonSet ps3;
	assign(ps3, ps1 * ps2);
	PolygonSet ps4;
	ps4 += ps1 + ps2;
	assert(area(ps4) == area(ps1) + area(ps2) - area(ps3));
	assert(equivalence((ps1 + ps2) - (ps1 * ps2), ps1 ^ ps2));
	rectangle_data<int> rect;
	assert(extents(rect, ps1 ^ ps2));
	assert(area(rect) == 225);
	assert(area(rect ^ (ps1 ^ ps2)) == area(rect) - area(ps1 ^ ps2));

	cv::Mat im(512, 512, CV_8UC3);

	cv::drawContours(im, ps1, -1, cv::Scalar(0, 0, 255), 1, CV_AA, cv::noArray(), 0, cv::Point(16,16));
	cv::drawContours(im, ps2, -1, cv::Scalar(255, 0, 0), 1, CV_AA, cv::noArray(), 0, cv::Point(16, 240));
	cv::drawContours(im, ps3, -1, cv::Scalar(0, 255, 0), 1, CV_AA, cv::noArray(), 0, cv::Point(240, 16));
	cv::drawContours(im, ps4, -1, cv::Scalar(255, 0, 255), 1, CV_AA, cv::noArray(), 0, cv::Point(240, 240));
	cv::imshow("hello", im);
	cv::waitKey(0);
}



int xrand(int x)
{
	return x + rand() % x - rand() % x;
}

static void thing_mouse_move(int event, int x, int y, int flags, void* userdata)
{
	vector<Point>& points = *reinterpret_cast<vector<Point>*>(userdata);
	Point mos(x, y);
	static int grab = -1;
	int mn = numeric_limits<int>::max();
	int mni = 0;
	
	switch (event)
	{
	case cv::EVENT_LBUTTONUP:
		grab = -1;
		break;
	case cv::EVENT_LBUTTONDOWN:
	{
		for (int i = 0; i < points.size(); ++i)
		{
			Point v = mos - points[i];
			int dot = v.dot(v);
			if (mn > dot)
			{
				mni = i;
				mn = dot;
			}	
		}
		if (mn < 64)
			grab = mni;
	}
		break;
	}
	if (grab >= 0)
		points[grab] = mos;

}
void run_thing(void)
{
	const int sz = 256;
	cv::Mat im(sz * 2, sz * 2, CV_8UC3);
	vector<Point> points = { 
		{ xrand(sz), xrand(sz) }, 
		{ xrand(sz), xrand(sz) }, 
		{ xrand(sz), xrand(sz) }
	};
	cv::namedWindow("boo");
	cv::setMouseCallback("boo", thing_mouse_move, &points);

	cv::Point cen(sz, sz);
	for (;;)
	{
		im.setTo(0);

		cv::Point2f a = points[0] - cen;
		cv::Point2f b = points[1] - cen;
		cv::Point2f q = points[2] - cen;

		a *= 1.f / sqrt(a.dot(a));
		b *= 1.f / sqrt(b.dot(b));
		q *= 1.f / sqrt(q.dot(q));
	
		
		float ab = a.dot(b);
		float aq = a.dot(q);

		//cout << a << b<<axb << bxa << endl;


		cv::line(im, cen, cv::Point2f(sz, sz + ab*sz*0.5), cv::Scalar(255, 255, 255), 1, CV_AA);
		cv::line(im, cen, cv::Point2f( sz + aq*sz*0.5,sz), cv::Scalar(255, 255, 255), 1, CV_AA);
		//cv::line(im, cen, cv::Point2f(sz, sz) + bxa*sz*0.5, cv::Scalar(255, 255, 255), 1, CV_AA);

		cv::line(im, cen, points[0], cv::Scalar(255, 0, 128), 1, CV_AA);
		cv::line(im, cen, points[1], cv::Scalar(128, 0, 255), 1, CV_AA);
		cv::line(im, cen, points[2], cv::Scalar(0, 200, 0), 1, CV_AA);

		cv::imshow("boo", im);
		int key = cv::waitKey(1);
		if (key >= 0)
			break;
	}
}



void test_manhatten(int n)
{
	const int n2 = n*n;

	for (int y = -n; y <= n; ++y)
	{
		for (int x = -n; x <= n; ++x)
		{
			int d = x + y;
			d *= d;
			printf("%c", (d > n2)? ' ': (d == n2 || x*x == n2 || y*y == n2) ? '#':'-' );

		}
		printf("\n");
	}
}


int main(int argc, char* argv[])
{

	//for (int x = -5674; x < 5000; x += 256)
	//{
	//	int y = x;
	//	hx::scale_down_safe(y, 256);
	//	printf("%d\t%d %d\n", x, x / 256,y );
	//}

	//return 0;
	mapped_image f;
	const int size = 6;
	//Point data[] = { { 0, 12 }, { 12, 0 }, { 6, 0 }, { 6, 6 } };
	Point data[] = { { 2, 7 }, { 7, 2 }, { 10, 5 }, { 5, 10 }, { 2, 7 } };
	//f.lines.push_back(vector<Point>(data,data+5));



	
	

	//for (auto& p : f.lines.front())
	//	cout << p << " ";
	//cout << endl;

	//for (auto& p : f.lines.front())
	//	Geometry60::reflect30(p,size);

	//for (auto& p : f.lines.front())
	//	cout << p << " ";
	//cout << endl;

	//for (auto& p : f.lines.front())
	//	Geometry60::reflect30(p, size);

	//for (auto& p : f.lines.front())
	//	cout << p << " ";
	//cout << endl;

	//test_polygon_set();
	//return 0;
	//printf("%f error\n", sqrt(3)-(double)MapperV::NUM/MapperV::DOM);



	bool quit = false;
	while (!quit)
	{
		f.draw();
		int key = f.show();
		switch (key)
		{
			case -1:
				break;
			case 'q':
				quit = true;
				break;
			case 'v':
				f.voronoi();
				break;
			case '+':
				for (auto& p : f.polyset_.front())
					Geometry60::rot_cw(p, cv::Point(size,size)); //Geometry60::rot_cw(p, size);
				break;
			case '-':
				for (auto& p : f.polyset_.front())
					Geometry60::rot_ccw(p, cv::Point(size, size)); //Geometry60::rot_ccw(p, size);
				break;
			case 'x':
				f.polyset_.clear();
				f.pline = 0;
				break;
			default:
				printf("%d '%c' unmapped", key, key);
				break;
		}
	}




	return 0;
}

