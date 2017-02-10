
#pragma once

#include <queue>
#include <SDL.h>
#include "AVReader.h"
extern "C"
{
#include "libavcodec/avcodec.h"  
#include "libavformat/avformat.h"  
};
class AVPlayer
{
private:
	double m_Time; //milisecond
	double m_VideoFps;
	std::queue<AVFrame*> m_VideoBuffer;
	std::queue<AVFrame*> m_AudioBuffer;

	int m_VideoBufferSize;
	int m_AudioBufferSize;

	AVReader * m_AVReader;

	//win
	SDL_Window * m_Window;
	SDL_Renderer* m_Renderer;

public:
	int OpenVideo(std::string fpath);

public:
	AVPlayer(SDL_Window * window);
	~AVPlayer();
};

