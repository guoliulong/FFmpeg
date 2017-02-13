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

	m_VideoTexture = SDL_CreateTexture(m_Renderer,
		SDL_PIXELFORMAT_RGB24, 
		SDL_TEXTUREACCESS_STREAMING,
		m_AVReader->getWidth(),
		m_AVReader->getHeight());
	SDL_assert(m_VideoTexture);

	m_RectVideo.h = m_AVReader->getHeight();
	m_RectVideo.w = m_AVReader->getWidth();
	m_RectVideo.x = 0;
	m_RectVideo.y = 0;

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
	int startT = ::SDL_GetTicks();
	//uint32_t qas = SDL_GetQueuedAudioSize(1);
	//printf("%d\n", qas);
	//if (qas < 4410 )
	{
		AVFrame* pcmFrame = av_frame_alloc();
		AVFrame* rawFrame = m_AVReader->receiveFrame(AVReader::AT_AUDIO);
		m_AVReader->convertAudio(pcmFrame, rawFrame);
		
		if (SDL_QueueAudio(1, pcmFrame->data[0], pcmFrame->nb_samples*2) == -1) //
		{
			printf("SDL_QueueAudio error");
		}

		m_Time = rawFrame->pts;

		av_frame_free(&pcmFrame);
		av_frame_free(&rawFrame);

		//printf("%f\n", m_Time);
		//printf("%d\n", ::SDL_GetTicks() - startT);
		//printf("%d\n" , qas);
	}
	//else
	{
		//printf("so muth speed");
	}

	//video
	AVFrame* pickFrame = m_AVReader->pickNextFrame(AVReader::AT_VIDEO);
	if (pickFrame->pts < m_Time)
	{
		AVFrame* rgbFrame = av_frame_alloc();
		AVFrame* yuvFrame = m_AVReader->receiveFrame(AVReader::AT_VIDEO);
		m_AVReader->convertVideo(rgbFrame, yuvFrame);

		SDL_assert(SDL_UpdateTexture(m_VideoTexture, &m_RectVideo, rgbFrame->data[0], yuvFrame->width * 3) == 0);
		SDL_Rect dstRect;
		memset(&dstRect, 0, sizeof(SDL_Rect));
		SDL_GetRendererOutputSize(m_Renderer, &dstRect.w, &dstRect.h);
		SDL_RenderCopy(m_Renderer, m_VideoTexture, &m_RectVideo, &dstRect);
		SDL_RenderPresent(m_Renderer);

		av_frame_free(&rgbFrame);
		av_frame_free(&yuvFrame);
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
