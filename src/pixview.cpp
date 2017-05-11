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
 */
class RenderCore {
private:
	cairo_surface_t *_image;
	Blob _blob;
//	_origin;
public:
	RenderCore(void);
//	RenderCore(const char *imageloc);
	bool loadImg(const char *imageloc);
	SDL_Surface *output2SDL(void);
};


RenderCore::RenderCore(void) :
	_image(NULL)
{}

//we will have sdl code here
bool
RenderCore::loadImg(const char *imageloc)
{
	//you need to clean up previous image as well.
	Image origin;
	origin.read(std::string(imageloc));
	origin.magick("PNG");
	origin.write(&_blob);
	if (_image)
		cairo_surface_destroy(_image);
	_image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, origin.columns(), origin.rows());
	origin.write(0, 0, origin.columns(), origin.rows(),
		     "RGBA", Magick::CharPixel,
		     cairo_image_surface_get_data(_image));
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
	const char *mime;
	//magic_t magic;
	
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
	int offset;
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
	int width, height;
	SDL_Window *win;
	SDL_Surface *win_surf;
	SDL_Surface *image;

	if (SDL_Init( SDL_INIT_VIDEO) == -1)
		return -1;
	
	//prepare everything before we show windows.
	RenderCore render;
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
	SDL_Event event;
	//this should be the main thread
	while (!shouldquit) {
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
					break;
				}
				//you have to get previous or next image
			} else if (event.type == SDL_MOUSEMOTION && event.motion.state == SDL_PRESSED) {
				std::cout << "mouse are moving" << std::endl;
			} else if (event.type == SDL_MOUSEWHEEL)
				std::cout << "wheel moving x: " << event.wheel.x << "and y: " << event.wheel.y << std::endl;
		}
		SDL_BlitSurface(image, NULL, win_surf, NULL);
		SDL_UpdateWindowSurface(win);
	}
	SDL_FreeSurface(image);
	SDL_FreeSurface(win_surf);
	return 0;	
}
