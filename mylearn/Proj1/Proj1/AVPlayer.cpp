#include "stdafx.h"
#include "AVPlayer.h"

//AVFrame * AVPlayer::AllocVideoFrame()
//{
//	SDL_assert(m_AVReader);
//	return av_frame_alloc();
//}
//
//AVFrame * AVPlayer::AllocAudioFrame()
//{
//	SDL_assert(m_AVReader);
//	return av_frame_alloc();
//}

int AVPlayer::OpenVideo(std::string fpath)
{
	m_AVReader = new AVReader(fpath.c_str());
	m_AVReader->init();
	//AVFrame* newFrame = m_AVReader->receiveFrame();

	SDL_Texture * texture = SDL_CreateTexture(m_Renderer,
		SDL_PIXELFORMAT_RGB24, 
		SDL_TEXTUREACCESS_STREAMING,
		m_AVReader->getWidth(),
		m_AVReader->getHeight());
	
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

	//init vars
	m_Time = 0;

	return 0;
}

int AVPlayer::Tick()
{
	uint32_t qas = SDL_GetQueuedAudioSize(1);

	if (qas < 1)
	{
		AVFrame* pcmFrame = av_frame_alloc();
		AVFrame* rawFrame = m_AVReader->receiveFrame(AVReader::AT_AUDIO);
		m_AVReader->convertAudio(pcmFrame, rawFrame);
		
		if (SDL_QueueAudio(1, pcmFrame->data[0], pcmFrame->nb_samples * 2) == -1) //newFrame->linesize[0]
		{
			printf("SDL_QueueAudio error");
		}
		av_frame_free(&pcmFrame);
		av_frame_free(&rawFrame);
	}
	else
	{
		printf("so muth speed");
	}

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
