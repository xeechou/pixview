#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <SDL2/SDL.h>
//#include <SDL2/SDL_image.h>
#include <cairo/cairo.h>

#include <iostream>
#include <string>
#include <thread>
#include <exception>
#include <Magick++.h>
#include <boost/filesystem.hpp>
//#include <magic.h>

#include "render.hpp"


using namespace Magick;

RenderCore::RenderCore(int maxw, int maxh) :
	_image(NULL), _w(maxw), _h(maxh)
{
	_pxratio_exp = 0;
}

RenderCore::~RenderCore()
{
	cairo_surface_destroy(_image);
	cairo_destroy(_cr);
}
//we will have sdl code here
bool
RenderCore::loadImg(const char *imageloc)
{
	Image second;
	//you need to clean up previous image as well.
	origin.read(std::string(imageloc));

	//scale image when necessary

	_ratio = adapt4window(_w, _h);
	second = scalebyRatio(_ratio);
	set2cairo(second);
	return true;
}

SDL_Surface *
RenderCore::output2SDL(void)
{
	int width, height;
	uint32_t rmask, gmask, bmask, amask;
	width = cairo_image_surface_get_width(_image);
	height = cairo_image_surface_get_height(_image);
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif
	//FIXME I am not sure what will happen if the sdl_surface here created get freed outside
	return SDL_CreateRGBSurfaceFrom((void *)cairo_image_surface_get_data(_image),
					width, height,
					32,
					cairo_image_surface_get_stride(_image),
					rmask, gmask, bmask, amask);
}


float
RenderCore::adapt4window(unsigned int w, unsigned int h)
{
	//this will also change the
	float ratiow, ratioh, ratio;
	ratiow = (w < origin.columns()) ? (float)w / (float) origin.columns() : 1;
	ratioh = (h < origin.rows()) ? (float)h / (float) origin.rows() : 1;
	ratio = std::min(ratiow, ratioh);
	return ratio;
}

//downscale, use the full image
Image
RenderCore::scalebyRatio(float ratio)
{
	Image second = origin;
	int rows, cols;
	
	rows = ratio * origin.rows(); cols = ratio * origin.columns();
	second.scale(Geometry(cols, rows));
	return second;
}
void RenderCore::set2cairo(Image& cropped)
{
	if (_image)
		cairo_surface_destroy(_image);
	_image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, cropped.columns(), cropped.rows());
	cropped.write(0,0,cropped.columns(), cropped.rows(), "RGBA", Magick::CharPixel, cairo_image_surface_get_data(_image));
}

SDL_Surface *
RenderCore::scale_slow(int ratio_inc, ImageGeo& geo)
{
	int imw = (int)origin.columns();
	int imh = (int)origin.rows();
	
	_pxratio_exp += ratio_inc;
	int sign = (_pxratio_exp > 0) - (_pxratio_exp < 0);
	float scale = std::pow(1 + sign * 0.1, std::abs(_pxratio_exp));
	geo.scaleImg(scale/_ratio);
	_ratio = scale;
	//new code
	//ensentially it will scale columns() * scale, rows() * scale much.
	//operate on image space.
	imw = (imw * scale > geo._w) ? _w : imw;
	imh = (imh * scale > geo._h) ? _h : imh;
	
	Image second = origin;
//	second.crop(Geometry(imw, imh,
//			     (bound.x < 0) ? 0 : bound.x,
//			     (bound.y < 0) ? 0 : bound.y));
	second.scale(Geometry(second.columns() * scale, second.rows() * scale));
	set2cairo(second);
	return output2SDL();
}

//thank god it is fast enough
SDL_Surface *
RenderCore::scale(int ratio_inc, ImageGeo2& geo)
{
	_pxratio_exp += ratio_inc;
	int sign = (_pxratio_exp > 0) - (_pxratio_exp < 0);
	float scale = std::pow(1 + sign * 0.1, std::abs(_pxratio_exp));
	geo.scale = scale;

	point2_t crop_origin, crop_end;
	geo.getcrop(crop_origin, crop_end);
	if (crop_origin.x > crop_end.x)
		return NULL;
	Image second = origin;
	second.crop(Geometry(crop_end.x - crop_origin.x,
			     crop_end.y - crop_origin.y,
			     crop_origin.x,
			     crop_origin.y));
	second.scale(Geometry(second.columns() * scale, second.rows() * scale));
	set2cairo(second);
	return  output2SDL();
	//you just need to find out about the crop
}


/*
SDL_Surface *
RenderCore::scale(int ratio_inc, ImageGeo& geo)
{
	//okay, it is still very slow, doesn't work
	int xoffset, yoffset;
	int imw = (int)origin.columns();
	int imh = (int)origin.rows();
	
	if (geo.outofbound(imw, imh))
		return  output2SDL();
	_pxratio_exp += ratio_inc;
	int sign = (_pxratio_exp > 0) - (_pxratio_exp < 0);
	float scale = std::pow(1 + sign * 0.1, std::abs(_pxratio_exp));
	//crop and scale it
	ImageGeo tmpgeo = geo.getScaledWinGeo(_ratio/scale);
	_ratio = scale;
	imw *= scale;
	imh *= scale;
	xoffset = (tmpgeo._x >= 0) ? 0 : -tmpgeo._x;
	yoffset = (tmpgeo._y >= 0) ? 0 : -tmpgeo._y;
	geo._x += xoffset; geo._y += yoffset;
	geo.scaleImg(scale/_ratio);

	Image second = origin;
	second.crop(Geometry(imw, imh, xoffset, yoffset));
	second.scale(Geometry(second.columns() * scale, second.rows() * scale));
	set2cairo(second);
	//crop image then scale it
	return output2SDL();
	//relocate your src rect
//	struct SDL_Rect srcRect;
}
*/

