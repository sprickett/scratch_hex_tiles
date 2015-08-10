
#include <opencv2\core\core.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <iostream>
using namespace std;

#include "Polygon.h"


struct VTri :public Point
{
	VTri(const Point& p = Point())
	:Point(p){};
};
struct HTri :public Point
{
	HTri(const Point& p = Point())
	:Point(p){};
};
struct VHex :public Point{};
struct HHex :public Point{};


class Mapping
{
public:
	Mapping(int size)
		:size_(size)
	{}

	HTri to_h_triangle(const Point& screen) const
	{
		return to_h_triangle(screen, size_);
	}
	VTri to_v_triangle(const Point& screen) const
	{
		return to_v_triangle(screen, size_);
	}
	template<class Typ>
	Point to_screen(const Typ& index) const
	{
		return to_screen(index, size_);
	}




	static HTri to_h_triangle(const Point& screen, int size)
	{
		HTri t;
		screen2HTri_(t.x, t.y, screen.x, screen.y, size);
		return t;
	}
	static VTri to_v_triangle(const Point& screen, int size)
	{
		VTri t;
		screen2VTri_(t.x, t.y, screen.x, screen.y, size);
		return t;
	}
	static Point to_screen(const HTri& triangle, int size)
	{
		Point p;
		hTri2Screen_(p.x, p.y, triangle.x, triangle.y, size);
		return p;
	}
	static Point to_screen(const VTri& triangle, int size)
	{
		Point p;
		vTri2Screen_(p.x, p.y, triangle.x, triangle.y, size);
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
	static inline void screen2HTri_(int& c, int& r, int x, int y, int size)
	{
		c = (Rt3DOM * (x + size)) / (Rt3NUM * size);
		r = y - size * c;
		r += size* sign(r); // offset for rounding
		r /= (size * 2);
	}
	static inline void hTri2Screen_(int& x, int& y, int c, int r, int size)
	{
		x = (size * c * Rt3NUM) / Rt3DOM;
		y = size * (2 * r + c);
	}
	static inline void screen2VTri_(int& c, int& r, int x, int y, int size)
	{
		r = (y + size) / (size*2);
		x *= Rt3NUM;
		c = Rt3DOM * 2 * size;
		x -= (r * c) ;
		x += c * sign(x);
		c = x / (c * 2);
	}
	static inline void vTri2Screen_(int& x, int& y, int c, int r, int size)
	{ 
		size *= 2;
		y = size * r;
		x = (size * Rt3DOM * (2 * c +  r)) / Rt3NUM;
	}


	int size_;
};

class MapperV
{
public:
	MapperV(int step = 15)
		: image_(900, 1600, CV_8UC3)
		,step_(step)
		, cols_(std::min(256, (image_.cols) / (step_ * 2)))
		, rows_(std::min(256, (image_.rows * DOM)/ (step_* NUM)))
		, ox_((image_.cols - (cols_*step_ * 2)) / 2)
		, oy_((image_.rows - (rows_*step_ * sqrt(3))) / 2)
		, my_(oy_ - (step_ * NUM) / DOM* 2)
		//, cols_(std::min(256, (image_.cols) / (step_ * 2)))
		//, rows_(std::min(256, int(image_.rows / (step_*rt3))))
		//, ox_((image_.cols - (cols_*step_ * 2)) / 2)
		//, oy_((image_.rows - (rows_*step_ * rt3)) / 2)
		//, my_(oy_ - step_ * rt3 * 0.5)
		, colour_(255,0,255)

	{

	}
	enum Root3
	{
		//NUM = 26, DOM = 15, //-0.00128253
		//NUM = 71, DOM = 41, //0.00034349
		//NUM = 97, DOM = 56, //-9.20496e-005
		//NUM = 265, DOM = 153, //2.46638e-005
		//NUM = 362, DOM = 209, //-6.6087e-006
		//NUM = 989, DOM = 571, //1.77079e-006
		//NUM = 1351, DOM = 780, //-4.74482e-007
		NUM = 3691, DOM = 2131, //1.27137e-007
		//NUM = 5042, DOM = 2911, //-3.40663e-008
		//NUM = 13775, DOM = 7953, //9.12804e-009
		//NUM = 18817, DOM = 10864, //-2.44585e-009
		//NUM = 51409, DOM = 29681, //6.55364e-010
		//NUM = 70226, DOM = 40545, //-1.75604e-010
		//NUM = 191861, DOM = 110771, //4.70528e-011
		//NUM = 262087, DOM = 151316, //-1.26079e-011
		//NUM = 716035, DOM = 413403, //3.37819e-012
		//NUM = 978122, DOM = 564719, //-9.05276e-013
		//NUM = 2672279, DOM = 1542841, //2.42473e-013
		//NUM = 3650401, DOM = 2107560, //-6.50591e-014
		//NUM = 9973081, DOM = 5757961, //1.73195e-014
		//NUM = 13623482, DOM = 7865521, //-4.66294e-015
	};



	cv::Vec2b screen2tri(cv::Point pixel) const
	{
		int x = pixel.x - ox_ + step_;
		int y = pixel.y- my_;
		y = (DOM * y) / (NUM*step_); 
		x -= step_*(y & 1);
		x /= step_ * 2;
		return cv::Vec2s(x,y);
	}

	cv::Point tri2screen(const cv::Vec2b& v) const
	{
		return cv::Point(
			ox_ + step_* (v[0] * 2 + (v[1] & 1)),
			oy_ + (step_ * v[1] * NUM) / DOM);
	}
	
	cv::Vec2b tri2hex(cv::Vec2b index, int size) //const
	{
		int x = index[0] / size;
		int rx = index[0] % size;
		int y = index[1] / (size*2);
		int ry = index[1] % (size * 2);

		int rx3 = x % 3;
		x /= 3;
		int oddx = x & 1;
		int fwd = (y & 1) ^ oddx;
		y = (y - oddx) / 2u;

		if (!rx3)
		{
			
			
			if (fwd)
			{
				if (rx * 2 <= ry)
				{
					y += oddx;
					--x;
				}
			}
			else
			{
				if ((size - rx) * 2 >= ry)
				{
					--x;
					y -= x & 1;
				}
			}
		}
		return cv::Vec2b(x,y);
	}
	cv::Vec2b hex2tri(cv::Vec2b index, int size) const
	{
		const int sz2 = size * 2;
		return cv::Vec2b(index[0] * size * 3 + sz2, (index[1] * 2 + 1 + (index[0]&1))  * sz2);
	}
	//cv::Vec2b get_hexagon(cv::Vec2b index) const
	//{
	//	int x = index[0] / 9;
	//	int rx = index[0] % 9;
	//	int y = (index[1] - (x & 1) * 6) / 12;
	//	return cv::Vec2b(x, y);
	//}


	//cv::Vec2b get_index(int x, int y) const
	//{
	//	x -= my_;//ox_ - step_;
	//	y -= oy_ - step_; 
	//	x = int(x / (rt3*step_)); // ! double to int	
	//	y -= step_*(x & 1);	
	//	y /= step_ * 2;
	//	return cv::Vec2b(x, y);
	//}
	//cv::Point get_point(const cv::Vec2b& v) const
	//{
	//	return cv::Point(
	//		ox_ + v[0] * rt3*step_,
	//		oy_ + (v[0] & 1)*step_ + step_*v[1] * 2);
	//}

	void clear(const cv::Scalar& colour = cv::Scalar())
	{
		image_.setTo(colour);
	}


	//void draw_line(const cv::Vec2b& p0, const cv::Vec2b& p1, const cv::Scalar& colour, int thickness = 1)
	//{
	//	draw_line(get_point(p0), get_point(p1), colour, thickness);
	//}


	void draw_point(const cv::Point& p, const cv::Scalar& colour)
	{
		image_.at<cv::Vec3b>(p) = cv::Vec3b(128, 128, 128);
	}
	void draw_line(const cv::Point& p0, const cv::Point& p1, const cv::Scalar& colour, int thickness=1)
	{
		cv::line(image_, p0, p1, colour, thickness, CV_AA);
	}
	void draw_circle(const cv::Point& p0, int radius, const cv::Scalar& colour, int thickness = 1)
	{
		cv::circle(image_, p0, radius, colour_, thickness, CV_AA);
	}
	void draw_text(const std::string& text, const cv::Point& p, double scale, const cv::Scalar& colour, int thickness = 1)
	{
		cv::putText(image_, text, p, cv::FONT_HERSHEY_SIMPLEX, scale, colour, thickness, CV_AA, false);
	}


	void draw_dots(const cv::Scalar& colour)
	{
		cv::Vec3b bgr(colour[0], colour[1], colour[2]);
		for (int y = 0;y<rows_; ++y)
		{
			for (int x = 0;x<cols_; ++x)
			{
				image_.at<cv::Vec3b>(tri2screen(cv::Vec2b(x, y))) = bgr;
			}
		}
	}



	void draw_hex_grid(int size, int ox, int oy, const cv::Scalar& colour = cv::Scalar(255, 255, 255))
	{
		int sml = size / 2;
		int big = (size + 1) / 2;

		repeat_line( ox + big,          oy + 0, size, 0,    size, colour);
		repeat_line( ox + size * 2,  oy + size, size, 0,    size, colour);
		repeat_line( ox + big,          oy + 0, -big, size, size, colour);
		repeat_line( ox + size * 2,  oy + size, -sml, size, size, colour);
		repeat_line( ox + 0,         oy + size,  big, size, size, colour);
		repeat_line( ox + size + big,   oy + 0,  sml, size, size, colour);
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
	void repeat_line( int ox, int oy, int vx, int vy, int size, const cv::Scalar& colour)
	{
		int xe = std::min(cols_, cols_-vx);
		int ye = std::min(rows_, rows_ - vy);
		for (int y = oy; y< ye; y += size * 2)
		{
			for (int x = ox; x <xe; x += size * 3)
			{
				draw_line(tri2screen(cv::Vec2b(x, y)), tri2screen(cv::Vec2b(x + vx, y + vy)), colour);
			}
		}
	}

	const static double rt3;
	std::string name_;
	cv::Mat image_;
	int step_;
	int hex_;
	int cols_;
	int rows_;
	int ox_;
	int oy_;
	int my_;
	cv::Scalar colour_;
	friend class MapperH;
};



class MapperH
{
public:
	MapperH(int step = 4)
		: image_(900, 1600, CV_8UC3)
		, tri_(step)
		, hex_(step * 9)
		, cols_(tri_.to_h_triangle(Point(image_.cols,0)).x)
		, rows_(tri_.to_h_triangle(Point(0, image_.rows)).y)
	{
		
	}
	


	HTri screen2tri(cv::Point pixel) const
	{
		return tri_.to_h_triangle(pixel);		
	}
	Point tri2screen(const HTri& t) const
	{ 
		return tri_.to_screen(t);
	}

	HTri screen2hex(cv::Point pixel) const
	{
		return hex_.to_v_triangle(pixel);
	}
	Point hex2screen(const VTri& t) const
	{
		return hex_.to_screen(t);
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


	void draw_dots(const cv::Scalar& colour)
	{
		cv::Vec3b bgr(colour[0], colour[1], colour[2]);
		HTri t= Point(0,0);
		for (t.x = 0; t.x<cols_; ++t.x)
		{
			t.y = -t.x / 2;
			int ry = rows_+ t.y;
			for (; t.y<ry; ++t.y)
			{
				cv::Point p = tri_.to_screen(t);
				if ((unsigned)p.x < image_.cols && (unsigned)p.y < image_.rows)
					image_.at<cv::Vec3b>(p) = bgr;
			}
		}


	}



	void draw_hex_grid(int size, int ox, int oy, const cv::Scalar& colour = cv::Scalar(255, 255, 255))
	{
		int sml = size / 2;
		int big = (size + 1) / 2;
		vector<HTri> line = {
			Point(size * 2, 0),
			Point(size, 0),
			Point(0, size),
			Point(0, size * 2),
		};

		repeat(line, Point(12, -6), colour);

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
		HTri p, v;
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
	void repeat(const vector<HTri>& line, HTri step, const cv::Scalar& colour)
	{
		{
		
			HTri rstep;// = step;
			rstep.x = step.y;
			rstep.y = step.x;//(step.x, step.y);
			for (int i = 1; i < line.size();++i)
			{
				HTri p0 = line[i-1];
				HTri p1 = line[i];

				for (int y = 0; y < 10; ++y)
				{
					for (int x = 0; x < 20; ++x)
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
		, pline(lines.end())
	{
		cv::namedWindow(name_);
		cv::setMouseCallback(name_, mouse_move_callback, this);
	}


	void draw(void)
	{
		scene_.clear();
		scene_.draw_dots(cv::Scalar(255, 0,0));
		//scene_.draw_hex_grid(3,1,0,cv::Scalar(128, 0, 0));
		scene_.draw_hex_grid(6, 0, 0, cv::Scalar(255, 0, 0));

		draw_lines();
	}

	int show()
	{
		int key = cv::waitKey(33);
		cv::imshow(name_, scene_.image());
		return key;
	}

	void mouse_move(int event, int x, int y, int flags)
	{
		mos_.x = x;
		mos_.y = y;

		switch (event)
		{
		case cv::EVENT_LBUTTONUP:
			if (pline != lines.end())
			{
				pline->push_back(scene_.screen2tri(mos_));
			}
			break;
		case cv::EVENT_LBUTTONDOWN:
			if (pline == lines.end())
			{
				lines.push_back(vector<HTri>());
				pline = lines.begin() + lines.size() - 1;
			}
			pline->push_back(scene_.screen2tri(mos_));
			break;
		case cv::EVENT_RBUTTONDOWN:
			if (pline != lines.end() && pline->size() < 2)
			{
				lines.erase(pline);
			}
			pline = lines.end();
			break;
		}



		//printf("%d %d %d %d\n", event, x, y, flags);
	}


private:

	void draw_lines(void)
	{
		const cv::Scalar line_colour = cv::Scalar(255, 0, 255);
		const cv::Scalar text_colour = cv::Scalar(255, 255, 255);


		cv::Mat image = scene_.image();
		for (int i = 0; i < lines.size(); ++i)
		{
			vector<HTri>& line = lines[i];
			cv::Point p0, p1 = scene_.tri2screen(line[0]);
			for (int i = 1; i < line.size(); ++i)
			{
				p0 = p1;
				p1 = scene_.tri2screen(line[i]);
				scene_.draw_line(p0, p1, cv::Scalar(255, 0, 255), 1);
			}
		}
		HTri itri = scene_.screen2tri(mos_);
		VTri ihex = scene_.screen2hex(mos_);

		Point tri = scene_.tri2screen(itri);
		Point hex = scene_.hex2screen(ihex);


		if (pline != lines.end())
			scene_.draw_line( scene_.tri2screen(pline->back()), tri, line_colour);
		scene_.draw_circle(tri, 5, cv::Scalar(255, 0, 255));
		scene_.draw_circle(hex, 12, line_colour);
		//scene_.draw_circle( , 12, line_colour);

		stringstream ss;
		ss << (int)itri.x << ", " << (int)itri.y << "(" <<  (int)ihex.x << ", " << (int)ihex.y <<")";
		scene_.draw_text(ss.str(), tri, 0.5, text_colour);

		ss = stringstream();
		ss << (int)ihex.x << ", " << (int)ihex.y;
		scene_.draw_text(ss.str(), hex, 0.5, text_colour);
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

	std::vector< std::vector< HTri > > lines;
	std::vector< std::vector<  HTri > >::iterator pline;


};



const double MapperV::rt3 = 1.7320508075688772935274463415059;


void test_polygon_set() 
{
	using namespace boost::polygon::operators;
	using namespace boost::polygon;
	PolygonSet ps1;
	ps1 += rectangle_data<int>(0, 0, 100, 100);
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




int main(int argc, char* argv[])
{

	//test_polygon_set();
	//return 0;
	//printf("%f error\n", sqrt(3)-(double)MapperV::NUM/MapperV::DOM);


	mapped_image f;


	do
	{
		f.draw();		
	} while (f.show() < 0);




	return 0;
}

