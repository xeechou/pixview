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

using namespace Magick;
namespace fs=boost::filesystem;


/*
 * all the operations are done with cairo, then blit into sdl surface.
 * This class is also a singlton as we are doing SDL initialization with it.
 * The main function of this class is changing image size
 */
class RenderCore {
	//you need to define the pixel size to draw 
private:
	Image origin; //this is the origin image, without scale
	cairo_surface_t *_image;
	unsigned int _w, _h; //max,
	float _ratio; //current ratio for image to show image

	int _pxratio_exp;
public:
	class ImageGeo {
	public:
		int _x, _y;  //image origin offset regards to window
		int _cx, _cy; //the current center of that image
		unsigned int _w, _h; //size of the window
		unsigned int _imw, _imh;
		ImageGeo(unsigned int x,  unsigned int y,
			 unsigned int wx, unsigned int wy,
			 unsigned int w,  unsigned int h,
			 unsigned int imw, unsigned int imh) :
			_w(w), _h(h), _imw(imw), _imh(imh) {
			_x = x; _y = y;
			//translation from window coordinates to image coordinates
			_cx = wx - x; _cy = wy - y;
		}
		bool outofbound(void) const {
			return (_cx < 0 || _cx > (int)_imw || _cy < 0 || _cy > (int)_imh);
		}
	};
	class point2_t {
	public:
		float x, y;
		point2_t(float x, float y) {this->x = x; this->y = y;}
		point2_t operator+(const point2_t& another) {
			return point2_t(this->x + another.x, this->y + another.y);
		}
		point2_t& operator*=(const float scale) {
			x *= scale;
			y *= scale;
//			w *= scale; h *= scale;
			return *this;
		}
	};
	RenderCore(int maxw, int maxh);
//	RenderCore(const char *imageloc);
	bool loadImg(const char *imageloc);
	SDL_Surface *output2SDL(void);
	void set2cairo(Image& img);
	
	Geometry intersect(const point2_t& orig, const point2_t& wh);

	//adapt image and window, this function is
	//used at the beginning of creating image, since we have
	//a limitation of windows size
	float adapt4window(unsigned int w, unsigned int h);
	//there are other cases when we need to change image size.
	Image scalebyRatio(float ratio);
//	Image scalebyPixSize(int pixsize);
	SDL_Surface *scale(int pixelsizeinc, const ImageGeo& geo);
};


RenderCore::RenderCore(int maxw, int maxh) :
	_image(NULL), _w(maxw), _h(maxh)
{
	_pxratio_exp = 0;
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
RenderCore::scale(int ratio_inc, const ImageGeo& geo)
{
	if (geo.outofbound())
		return  output2SDL();
	//so the algorithm goes like this: we need to find the part of image
	//that actually worth scaling, we crop that area then thransfer it back.
	
	//now, get the scale
	_pxratio_exp += ratio_inc;
	int sign = (_pxratio_exp > 0) - (_pxratio_exp < 0);
	float scale = std::pow(1 + sign * 0.1, std::abs(_pxratio_exp));

	//translate window to center of scale
	point2_t win_origin(-geo._cx + geo._x, -geo._cy + geo._y);
	point2_t wh(geo._w, geo._h);
	
	//scale
	win_origin *= 1.0 / scale;
	wh *= 1.0 / scale;
	//translate to origin of image
	win_origin.x -= geo._x; win_origin.y -= geo._y;
	wh.x -= geo._x; wh.y -= geo._y;
	//intersect 
	Geometry tocrop = intersect(win_origin, wh);
	Image cropped = origin;
	cropped.crop(tocrop);
	cropped.scale(Geometry(cropped.columns() * scale, cropped.rows() * scale));
	set2cairo(cropped);
	//relocate your src rect
	struct SDL_Rect srcRect;
			
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

static int MAIN_PID = -1;

/**
 * return the offset of that file, and stores the result
 */
static int
get_images_in_dir(const std::string& img, std::vector<std::string>& imgs)
{
	imgs.clear();
	if (!fs::path(img).is_absolute())
	return -1;
	//now, you need to make two list, then concate them together
	std::vector<fs::path> imgs_before;
	std::vector<fs::path> imgs_after;

	for (fs::directory_iterator itr(fs::path(img).parent_path());
	     itr != fs::directory_iterator();
	     ++itr)
		imgs.push_back(itr->path().string());
	std::sort(imgs.begin(), imgs.end());
	return std::distance(imgs.begin(), std::find(imgs.begin(), imgs.end(), img));
}

static std::string
get_abs_path(const char* img_path)
{
	const char *current_path;
	fs::path p(img_path);
		
	//if your
	if (p.is_absolute())
		return p.string();
	else 
		return (fs::path(getenv("PWD")) / p.filename()).string();
}

static int
get_prevnext_img(const std::vector<std::string>& files, int offset, int direction)
{
	Image toload;
	bool foundimage = false;
	while (!foundimage) {
		offset += direction;
		if (offset < 0)
			offset += files.size();
		else if (offset >= files.size())
			offset -= files.size();
		try {
			toload.ping(files[offset]);
		} catch (std::exception & error) {
			//okay, this is not a image.
			continue;
		}
		foundimage = true;
	}
	return offset;
}

int main(int argc, char **argv)
{
	int offset, maxw, maxh;
	bool shouldquit = false;
	std::string absimg_path;
	std::vector<std::string> imgs;
	MAIN_PID = getpid();
	InitializeMagick(*argv);

	absimg_path = get_abs_path(argv[1]);
	offset = get_images_in_dir(absimg_path, imgs);
	//TODO: has more option processing
	if (offset == imgs.size() || offset < 0) {
		std::cerr << "Invalid image: " << argv[1] << std::endl;
		return -1;
	}
//	int width, height;
	SDL_Window *win;
	SDL_Surface *win_surf;
	SDL_Surface *image;

	if (SDL_Init( SDL_INIT_VIDEO) == -1)
		return -1;
	
	for (int i = 0; i < SDL_GetNumVideoDisplays(); i++) {
		SDL_DisplayMode current;
		
		int shouldbezero = SDL_GetCurrentDisplayMode(i, &current);
		if (shouldbezero)
			return -1;
		maxw = current.w * 0.6;
		maxh = current.h * 0.6;
		break;
	}
	
	//prepare everything before we show windows.
	RenderCore render(maxw, maxh);
	render.loadImg(absimg_path.c_str());
	image = render.output2SDL();
	if ((win = SDL_CreateWindow(argv[1],
				    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
				    image->w, image->h,
				    //or we probably should remain hidden right now
				    SDL_WINDOW_SHOWN) ) == NULL) {
		perror("unable to create a window\n");
		return -1;
	}
	//show
	win_surf = SDL_GetWindowSurface(win);
	SDL_BlitSurface(image, NULL, win_surf, NULL);
	
	SDL_Event event;
	SDL_Rect srcRect = {
		.x = 0,
		.y = 0,
	};			
	
	//so this is actually the origin of image 
	while (!shouldquit) { 
		//TODO: fix 100% cpu occupy
		while(SDL_PollEvent(&event)) {
			//so yes, right now we need to
			if (event.type == SDL_QUIT)
				shouldquit = true;
			else if (event.type == SDL_KEYUP) {
				switch (event.key.keysym.sym) {
				case SDLK_LEFT:
					std::cout << "left key pressed" << std::endl;
					offset = get_prevnext_img(imgs, offset, -1);
					render.loadImg(imgs[offset].c_str());
					SDL_FreeSurface(image);
					image = render.output2SDL();
					SDL_SetWindowSize(win, image->w, image->h);
					SDL_FreeSurface(win_surf);
					win_surf = SDL_GetWindowSurface(win);
					SDL_BlitSurface(image, NULL, win_surf, NULL);
					srcRect.x = srcRect.y = 0;
					break;
				case SDLK_RIGHT:
					std::cout << "right key pressed" << std::endl;					
					offset = get_prevnext_img(imgs, offset, 1);
					render.loadImg(imgs[offset].c_str());
					SDL_FreeSurface(image);
					image = render.output2SDL();
					SDL_SetWindowSize(win, image->w, image->h);
					SDL_FreeSurface(win_surf);
					win_surf = SDL_GetWindowSurface(win);
					SDL_BlitSurface(image, NULL, win_surf, NULL);
					srcRect.x = srcRect.y = 0;
					break;
				}
				//you have to get previous or next image
			} else if (event.type == SDL_MOUSEMOTION) {
				bool outofbound = false;
				int w, h;
				int posx, posy;

				
				SDL_GetWindowSize(win, &w, &h);
				posx = event.motion.x; posy = event.motion.y;
				outofbound = outofbound | (posx < 0 || posx > w);
				outofbound = outofbound | (posy < 0 || posy > h);
				if (event.motion.state != SDL_PRESSED || outofbound)
					continue;
//				std::cout << "before moving" << destRect.x << " " << destRect.y << std::endl;
//				destRect.x += event.motion.xrel;
//				destRect.y += event.motion.yrel;
//				destRect.w = image->w; destRect.h = image->h;
				srcRect.x -= event.motion.xrel;
				srcRect.y -= event.motion.yrel;
				srcRect.w = image->w; srcRect.h = image->h;
//				srcRect.x = std::max(srcRect.x, 0);				
//				srcRect.y = std::max(srcRect.y, 0);
				//sdl does clipping, so you need to use srcRect,
//				std::cout << "after moving " << srcRect.x << " " << srcRect.y << std::endl;
				SDL_FillRect(win_surf, NULL, 0x000000);
				SDL_BlitSurface(image, &srcRect, win_surf, NULL);
			} else if (event.type == SDL_MOUSEWHEEL) {
				std::cout << "wheel moving x: " << event.wheel.x << "and y: " << event.wheel.y << std::endl;
			}

		}

		SDL_UpdateWindowSurface(win);
	}
	SDL_FreeSurface(image);
	SDL_FreeSurface(win_surf);
	return 0;	
}
