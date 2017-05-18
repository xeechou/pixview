#include <SDL2/SDL.h>
#include "render.hpp"

#include <iostream>

void
ImageGeo2::shitft(int xoffset, int yoffset) {
	_wcx += xoffset;
	_wcy += yoffset;
	//wait, _cx, _cy shouldn't change
	//_cx -= xoffset * 1.0/scale;
	//_cy -= xoffset * 1.0/scale;
}

void
ImageGeo2::setImgCent(int x, int y) {
	int xoffset = x - _wcx;
	int yoffset = y - _wcy;
	_wcx = x; _wcy = y;
	//determine the new image center, 
	_cx += xoffset / scale;
	_cy += yoffset / scale;
//	std::cout << "center of image is now: " << _cx << " " << _cy << std::endl;
}

SDL_Rect
ImageGeo2::cast2SrcRect(void) {
	//firstly you need to know about crop

	int destX, destY;
	int new_cx = _cx * scale;
	int new_cy = _cy * scale;

	point2_t crop_begin, crop_end;
	this->getcrop(crop_begin, crop_end);
	//translate to the crop space;
	new_cx -= crop_begin.x * scale;
	new_cy -= crop_begin.y * scale;
		
	//moving from center to the origin
	destX = new_cx - _wcx;
	destY = new_cy - _wcy;

	//the src rect is not correct....
	SDL_Rect srcRect = {
		.x = -destX,
		.y = -destY,
		.w = (int)_ww,
		.h = (int)_wh
	};	
	//is it true?
	return srcRect;
}

void
ImageGeo2::getcrop(point2_t &orig, point2_t &end)
{
	point2_t win_orig = this->winp2imgp(point2_t(0, 0));
	point2_t win_end  = this->winp2imgp(point2_t(_ww, _wh));

	orig.x = std::max(0.0f, win_orig.x);
	orig.y = std::max(0.0f, win_orig.y);
	end.x  = std::min(win_end.x, (float)_imw);
	end.y  = std::min(win_end.y, (float)_imh);
}
