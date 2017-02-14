// ConsoleApplication2.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include <SDL.h>
#include<Windows.h>
#include "avreader.h"
#include"AVPlayer.h"
//extern bool saveBmp(char *bmpName, unsigned char *imgBuf, int width, int height,
//	int biBitCount, RGBQUAD *pColorTable);
//void SaveAsBMP(AVFrame *pFrameRGB, int width, int height, int index, int bpp);
//
class AVReader;
//Screen dimension constants
const int SCREEN_WIDTH = 480;
const int SCREEN_HEIGHT = 240;

int main(int argc, char* args[])
{
	//test
	//_CrtDumpMemoryLeaks();
	//AVFrame* pcmFrame = av_frame_alloc();
	//pcmFrame->data[8] = (uint8_t*)malloc(10);
	//av_frame_free(&pcmFrame);

	//_CrtDumpMemoryLeaks();



	//The window we'll be rendering to
	SDL_Window* window = NULL;

	SDL_Renderer* renderer;
	//The surface contained by the window
	SDL_Surface* screenSurface = NULL;

	AVReader* avReader;

	//XAVDataBuffer m_audio_data_buffer(1024 * 8);

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}
	else
	{
		//Create window
		window = SDL_CreateWindow("FFMEPG", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
		if (window == NULL)
		{
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		}
		else
		{
			/*renderer = SDL_CreateRenderer(window, -1, 0);
			SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
			SDL_RenderClear(renderer);
			SDL_RenderPresent(renderer);*/
			

			AVPlayer player(window);
			//player.OpenVideo("C:\\1.MOV");
			player.OpenVideo("C:\\阿拉丁.Aladdin.1992.HDTV.2Audio.MiniSD-TLF.mkv");
			SDL_Event event;
			//Wait two seconds
			while (1)
			{
				player.Tick();
				
				while (SDL_PollEvent(&event))
				{
					switch (event.type)
					{
					case SDL_QUIT:
						goto QUIT;
						break;
					default:
						break;
					}
				}
			}
		}
	}

QUIT:

	//Destroy window
	SDL_DestroyWindow(window);

	//Quit SDL subsystems
	SDL_Quit();

	return 0;
}


