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
namespace fs=boost::filesystem;



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

	//data that we keep, may moved to other places
	SDL_Event event;
	SDL_Rect srcRect = {
		.x = 0,
		.y = 0,
	};			
	int w, h;
	int posx, posy;
	SDL_GetWindowSize(win, &w, &h);	
	ImageGeo2 geo2(point2_t(w, h), render.getImgSize());
	
	while (!shouldquit) {
		//TODO: fix 100% cpu occupy
		while(SDL_PollEvent(&event)) {

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
					geo2.scale = 1;
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
					geo2.scale = 1;
					break;
				}
				//you have to get previous or next image
			} else if (event.type == SDL_MOUSEMOTION) {
				bool outofbound = false;
				
				SDL_GetWindowSize(win, &w, &h);
				posx = event.motion.x; posy = event.motion.y;
				geo2.setImgCent(posx, posy);
				outofbound = outofbound | (posx < 0 || posx > w);
				outofbound = outofbound | (posy < 0 || posy > h);
				if (event.motion.state != SDL_PRESSED || outofbound)
					continue;
				geo2.shitft(event.motion.xrel, event.motion.yrel);
//				srcRect = geo2
//				destRect.x += event.motion.xrel;
//				destRect.y += event.motion.yrel;
//				destRect.w = image->w; destRect.h = image->h;
				srcRect.x -= event.motion.xrel;
				srcRect.y -= event.motion.yrel;
				srcRect.w = w; srcRect.h = h;
//				srcRect.x = std::max(srcRect.x, 0);				
//				srcRect.y = std::max(srcRect.y, 0);
				//sdl does clipping, so you need to use srcRect,
//				std::cout << "after moving " << srcRect.x << " " << srcRect.y << std::endl;
				SDL_FillRect(win_surf, NULL, 0x000000);
				SDL_BlitSurface(image, &srcRect, win_surf, NULL);
			} else if (event.type == SDL_MOUSEWHEEL) {
//				ImageGeo geo(-srcRect.x, -srcRect.y, posx, posy, w, h);
				if (image)
					SDL_FreeSurface(image);

				image = render.scale((event.wheel.y * -1 < 0) ? 1 : -1,  geo2);
				//okay, the srcRect is not currect. gonna fix it
//				std::cout << srcRect.x << " beforesrcRect " << srcRect.y << std::endl;


				srcRect = geo2.cast2SrcRect();
				geo2.setImgCent(posx, posy);							
//				std::cout << srcRect.x << " srcRect " << srcRect.y << std::endl << std::endl;
				SDL_FillRect(win_surf, NULL, 0x000000);
				SDL_BlitSurface(image, &srcRect, win_surf, NULL);
//				std::cout << "wheel moving x: " << event.wheel.x << "and y: " << event.wheel.y << std::endl;
			}
		}
		SDL_UpdateWindowSurface(win);
	}
	SDL_FreeSurface(image);
	SDL_FreeSurface(win_surf);
	return 0;	
}
