#ifndef RENDER_HPP
#define RENDER_HPP

#include <SDL2/SDL.h>
//#include <SDL2/SDL_image.h>
#include <cairo/cairo.h>
#include <Magick++.h>


/**
 * Simple 2D point class with operator overrider
 */
class point2_t {
public:
	float x, y;
	point2_t(float x, float y) {this->x = x; this->y = y;}
	point2_t() {x = 0; y = 0;}
	point2_t operator+(const point2_t& another) {
		return point2_t(this->x + another.x, this->y + another.y);
	}
	point2_t operator+=(const point2_t& another) {
		y += another.y;
		x += another.x;
		return *this;
	}
	point2_t& operator*=(const float scale) {
		x *= scale;
		y *= scale;
//			w *= scale; h *= scale;
		return *this;
	}
};

struct geometry {
	point2_t o;
	point2_t wh;
};

class ImageGeo {
	//the image location regarding to current window,
	//the image size however is not recorded.
public:
	int _x, _y;  //image origin offset regards to window
	int _cx, _cy; //the current center of that image
	unsigned int _w, _h; //size of the window
	float scale;
	//to avoid concatenated scale problem, we move everything here
	ImageGeo(unsigned int x,  unsigned int y,
		 unsigned int wx, unsigned int wy,
		 unsigned int w,  unsigned int h) :
		_w(w), _h(h) {
		_x = x; _y = y;
		//translation from window coordinates to image coordinates
		_cx = wx - x; _cy = wy - y;
	}
	ImageGeo(const ImageGeo& copy) {
		_x = copy._x; _y = copy._y;
		_cx = copy._cx; _cy = copy._cy;
		_w = copy._w; _h = copy._h;
	}
	
	bool outofbound(unsigned int _imw, unsigned int _imh) const {
		return (_cx < 0 || _cx > (int)_imw || _cy < 0 || _cy > (int)_imh);
	}
	SDL_Rect cast2srcRect() {
		//int wx = _x + _cx;
		//int wy = _y + _cy;
		SDL_Rect srcRect = {
			.x = -_x,
			.y = -_y,
			.w = (int)_w,
			.h = (int)_h
		};	
		return srcRect;
	};
	point2_t wcp() {return point2_t(_cx + _x, _cy + _y);}
	
	//when image changes, you need to update accordingly, with these
	//methods, the code should be way easier to write
	//geometry transform code
	/*
	point2_t winp2imgp(const point2_t& winp) {
		return point2_t(winp.x - _x, winp.y - _y);
	}
	point2_t imgp2winp(const point2_t& imgp) {
		return point2_t(imgp.x + _x, imgp.y + _y);
	}
	point2_t imgp2centp(const point2_t& imgp) {
		return point2_t(imgp.x - _cx, imgp.y - _cy);
	}
	point2_t winp2centp(const point2_t& winp) {
//		point2_t imgp = winp2imgp(winp);
		return imgp2centp(winp2imgp(winp));
	}
	point2_t centp2imgp(const point2_t& cp) {
		return point2_t(cp.x + _cx, cp.y + _cy);
	}
	point2_t centp2winp(const point2_t& cp) {
		//move cp to imgp first
		point2_t imgp(cp.x + _cx, cp.y + _cy);
		return imgp2winp(imgp);
	}
	*/
//	point2_t 
	//scale image and update the image and window coordinates 
	void scaleImg(float scale) {
		point2_t wcp(_cx + _x, _cy + _y);
		point2_t img_orig(-_cx, -_cy);
		img_orig *= scale;
		img_orig += wcp;
		_x = img_orig.x;
		_y = img_orig.y;
	}
	ImageGeo getScaledImgGeo(float scale) {
		ImageGeo geo = *this;
		geo.scaleImg(scale);
		return geo;
	}
	void scaleWin(float scale) {
		//center coord system
		point2_t win_orig(-_cx - _x, -_cy - _y);
		//center coord_system
		point2_t img_orig(-_cx, -_cy);
		//change to win coord
		win_orig *= scale;
		_w *= scale; _h *= scale;
		_x = img_orig.x - win_orig.x;
		_y = img_orig.y - win_orig.y;
	}
	ImageGeo getScaledWinGeo(float scale) {
		ImageGeo geo = *this;
		geo.scaleWin(scale);
		return geo;
	}
	//TODO wront code get intersect area of image with current window, and update every thing
	Magick::Geometry intersect(unsigned int imw, unsigned int imh) {
		point2_t wcp = this->wcp();
		int x = (_x < 0) ? -_x : 0;
		int y = (_y < 0) ? -_y : 0;

		_x = std::max(0, _x);
		_y = std::max(0, _y);
		imw = std::min(imw, _w +1);
		imh = std::min(imh, _h +1);
		_cx = wcp.x - _x;
		_cy = wcp.y - _y;

		return Magick::Geometry(x,y,imw,imh);
	}
};



class ImageGeo2 {
public:	
	//image center in the image coordinates, but it is not reflect to wcp();
	int _cx, _cy; 
	//where your cursor is
	int _wcx, _wcy;
	//_cx, _wcx are essentially the same thing
	
	//how big is your image
	int _imw, _imh;
	//how big is your window
	int _ww, _wh;
	float scale;
	//methods
	
	ImageGeo2(const point2_t& wwh, const point2_t& iwh) :
		_cx(0), _cy(0), _wcx(0), _wcy(0), scale(1.0) {
		_imw = iwh.x;
		_imh = iwh.y;
		_ww  = wwh.x;
		_wh  = wwh.y;
	}
	//compiler will declare a copy constructor for me
	
	//dragging function
	void shitft(int xoffset, int yoffset);
	//there must be a hard part, here it is
	void setImgCent(int x, int y);
	//in the image coordinates
	SDL_Rect cast2SrcRect(void);

	point2_t winp2imgp(const point2_t& winp) {
		//winp -> (- _wcx ) ->
		return point2_t((winp.x - _wcx) / scale + _cx,
				(winp.y - _wcy) / scale + _cy);
	}
	point2_t imgp2winp(const point2_t& imgp) {
		return point2_t((imgp.x - _cx) * scale + _wcx,
				(imgp.y - _cy) * scale + _wcy);
	}
	void getcrop(point2_t& orig, point2_t& end);
};



/*
 * all the operations are done with cairo, then blit into sdl surface.
 * This class is also a singlton as we are doing SDL initialization with it.
 * The main function of this class is changing image size
 */
class RenderCore {
	//you need to define the pixel size to draw 
private:
	Magick::Image origin;
	unsigned int _w, _h; //max,
	float _ratio; //current ratio for image to show image
	int _pxratio_exp;
	//cairo data
	cairo_t *_cr;
	
	cairo_surface_t *_image;
public:
	RenderCore(int maxw, int maxh);
	~RenderCore(void);
//	RenderCore(const char *imageloc);
	point2_t getImgSize() {
		return point2_t(origin.columns(), origin.rows());
	}
	bool loadImg(const char *imageloc);
	SDL_Surface *output2SDL(void);
	void set2cairo(Magick::Image& img);
	
	Magick::Geometry intersect(const point2_t& orig, const point2_t& wh);

	//adapt image and window, this function is
	//used at the beginning of creating image, since we have
	//a limitation of windows size
	float adapt4window(unsigned int w, unsigned int h);
	//there are other cases when we need to change image size.
	Magick::Image scalebyRatio(float ratio);
//	Image scalebyPixSize(int pixsize);
//	SDL_Surface *scale(int pixelsizeinc, ImageGeo& geo);
	SDL_Surface *scale_slow(int pixelsizeinc, ImageGeo& geo);
	SDL_Surface *scale(int ratio_inc, ImageGeo2& geo);
};


#endif
