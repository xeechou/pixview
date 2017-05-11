#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
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
{
}

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

static std::string get_abs_path(const char* img_path)
{
	const char *current_path;
	fs::path p(img_path);
		
	//if your
	if (p.is_absolute())
		return p.string();
	else 
		return (fs::path(getenv("PWD")) / p.filename()).string();
}


int main(int argc, char **argv)
{
	int offset;
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
	if ( !(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) ) {
		perror("failed to init png routine\n");
		return -1;
	}
	
	//prepare everything before we show windows.
	RenderCore render;
	render.loadImg(argv[1]);
	image = render.output2SDL();	
//	cairo_image_surface_create_for_data(image->pixels, CAIRO_FORMAT_RGB24, image->w, image->h, image->pitch);
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
	SDL_UpdateWindowSurface(win);
	SDL_Delay(2000);
	return 0;
//	for_cairo = SDL_ConvertSurface(image, win_surf->format, 0);
//	SDL_FreeSurface(image);
	//so it looks like you always have a at least two surface, then use blitsurface to copy to the window surface	
//	SDL_BlitSurface(for_cairo, NULL, win_surf, NULL);
//	SDL_UpdateWindowSurface(win);
//	SDL_Delay(2000);

	//then you destroy the surface
//	SDL_FreeSurface(for_cairo);
//	SDL_FreeSurface(win_surf);
//	SDL_DestroyWindow(win);
//	for_cairo = SDL_ConvertSurface(SDL_Surface *src, const SDL_PixelFormat *fmt, Uint32 flags)
//	while(1);
		

//	sdlsurf = 
	
//	SDL_DisplayMode current;
//	for (int i = 0; i < SDL_GetNumVideoDisplays(); i++) {
//		int should_be_zero = SDL_GetCurrentDisplayMode(i, &current);
//	}
	
	//create a surface from a png file

	//cr = cairo_create(surface);
//	image_indexer.join();
}
