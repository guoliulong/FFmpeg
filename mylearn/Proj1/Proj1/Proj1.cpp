// ConsoleApplication2.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include <SDL.h>
#include<Windows.h>
#include "avreader.h"

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
	//The window we'll be rendering to
	SDL_Window* window = NULL;

	SDL_Renderer* renderer;
	//The surface contained by the window
	SDL_Surface* screenSurface = NULL;
	
	AVReader* avReader;
	
	XAVDataBuffer m_audio_data_buffer(1024*8);

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO| SDL_INIT_AUDIO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}
	else
	{
		//Create window
		window = SDL_CreateWindow("FFMEPG", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN| SDL_WINDOW_RESIZABLE);
		if (window == NULL)
		{
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		}
		else
		{
			//set audio
			//SDL_AudioSpec


			//Get window surface
			//screenSurface = SDL_GetWindowSurface(window);
			renderer = SDL_CreateRenderer(window, -1, 0);
			SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
			SDL_RenderClear(renderer);
			SDL_RenderPresent(renderer);
			//Fill the surface white
			//SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0, 0xFF, 0xFF));

			//Update the surface
			//SDL_UpdateWindowSurface(window);

			//read mp4
			//avReader = new AVReader("C:\\EP01.mp4");
			avReader = new AVReader("G:\\愤怒的小鸟.BD1280超清英国台粤四语中英双字.mp4");
			avReader->init();
			AVFrame* newFrame = avReader->receiveFrame();
			
			SDL_Texture * texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, newFrame->width, newFrame->height);
			SDL_Rect rectSrc;
			rectSrc.w = newFrame->width;
			rectSrc.h = newFrame->height;
			rectSrc.x = 0;
			rectSrc.y = 0;
			SDL_Rect rectDest;
			rectDest.w = SCREEN_WIDTH;
			rectDest.h = SCREEN_HEIGHT;
			rectDest.x = 0;
			rectDest.y = 0;
			
			
			//auido
			SDL_AudioSpec aSpec,retSpec;
			aSpec.callback = NULL;
			aSpec.channels = 2;
			aSpec.format = AUDIO_U8;
			aSpec.freq = 44100;
			aSpec.padding = 0;
			aSpec.samples = 0;
			aSpec.silence = 0;
			aSpec.size = 0;
			aSpec.userdata = NULL;

			if (SDL_OpenAudio(&aSpec, &retSpec) !=0)
			{
				printf("Open audio failed");
				return -1;
			}

			SDL_PauseAudio(0);

			//void* sampleBuffer = malloc(retSpec.size);

			int ret;
			SDL_Event event;
			//Wait two seconds
			while (1)
			{
				if (!newFrame)
				{
					goto QUIT;
				}

				if (newFrame->nb_samples == 0)
				{
					ret = SDL_UpdateTexture(texture, &rectSrc, newFrame->data[0], newFrame->width * 3);
					SDL_GetRendererOutputSize(renderer, &rectDest.w, &rectDest.h);
					SDL_RenderCopy(renderer, texture, &rectSrc, &rectDest);
					SDL_RenderPresent(renderer);
				}
				else
				{
					if (true)
					{
						//m_audio_data_buffer.Append(newFrame->data[0], newFrame->linesize[0]);

						//if (m_audio_data_buffer.GetSize() >= retSpec.size)
						{
							//m_audio_data_buffer.ReadFront(sampleBuffer, retSpec.size);

							static int i = 0;

							//if(i ==0)
							//if (SDL_QueueAudio(1, sampleBuffer, retSpec.size) == -1)
							if (SDL_QueueAudio(1, newFrame->data[0], newFrame->nb_samples*2) == -1) //newFrame->linesize[0]
							{
								printf("SDL_QueueAudio error");
							}

							++i;
						}
					}
				}

				newFrame = avReader->receiveFrame();

				while (SDL_PollEvent(&event))
				{
					switch (event.type)
					{
						//case SDL_KEYDOWN: 
						/* Any keypress quits the app... */
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


