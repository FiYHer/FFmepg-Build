
#include <stdio.h>
#define __STDC_CONSTANT_MACROS
#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "SDL.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <SDL.h>
#ifdef __cplusplus
};
#endif
#endif
//ffmpeg
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"postproc.lib")
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avfilter.lib")
#pragma comment(lib,"avdevice.lib")
//sdl2
#pragma comment(lib,"x86/SDL2.lib")
#pragma comment(lib,"x86/SDL2main.lib")
#pragma comment(lib,"x86/SDL2test.lib")
#pragma comment(linker, "/subsystem:\"console\" /entry:\"mainCRTStartup\"")
int main(int agrc, char* zrgv[])
{

	SDL_Window *window;
	SDL_Init(SDL_INIT_EVERYTHING);
	window = SDL_CreateWindow(
		"An SDL2 window",                  // window title
		SDL_WINDOWPOS_UNDEFINED,           // initial x position
		SDL_WINDOWPOS_UNDEFINED,           // initial y position
		640,                               // width, in pixels
		480,                               // height, in pixels
		SDL_WINDOW_OPENGL                  // flags - see below
	);

	if (window == NULL)
	{
		printf("Could not create window: %s\n", SDL_GetError());
		return 1;
	}
	SDL_Delay(3000);
	SDL_DestroyWindow(window);
	SDL_Quit();
	system("pause");
	return 0;
}
