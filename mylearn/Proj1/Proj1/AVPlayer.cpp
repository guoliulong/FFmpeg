#include "stdafx.h"
#include "AVPlayer.h"

int AVPlayer::OpenVideo(std::string fpath)
{
	m_AVReader = new AVReader("E:\\KwDownload\\song\\衡越-谁是我的新娘(《乡村爱情》内地剧片尾曲).mp3");
	m_AVReader->init();
	AVFrame* newFrame = m_AVReader->receiveFrame();

	SDL_Texture * texture = SDL_CreateTexture(m_Renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, newFrame->width, newFrame->height);
	
	//auido
	SDL_AudioSpec aSpec, retSpec;
	aSpec.callback = NULL;
	aSpec.channels = 2;
	aSpec.format = AUDIO_U8;
	aSpec.freq = 44100;
	aSpec.padding = 0;
	aSpec.samples = 0;
	aSpec.silence = 0;
	aSpec.size = 0;
	aSpec.userdata = NULL;

	if (SDL_OpenAudio(&aSpec, &retSpec) != 0)
	{
		printf("Open audio failed");
		return -1;
	}

	SDL_PauseAudio(0);


	return 0;
}

AVPlayer::AVPlayer(SDL_Window * window)
{
	SDL_assert(window);

	m_VideoBufferSize = 5;
	m_AudioBufferSize = 5;
	m_Window = window;

	m_Renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_assert(m_Renderer);
}


AVPlayer::~AVPlayer()
{
}
