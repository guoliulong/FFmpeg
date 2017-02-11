
#pragma once
#include <queue>
extern "C"
{
#include "libavcodec/avcodec.h"  
#include "libavformat/avformat.h"  
#include "libswscale/swscale.h"  
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
};

class AVReader
{
public:

	enum AV_TYPE
	{
		AT_AUDIO,
		AT_VIDEO
	};

private:
	AVFormatContext *pFormatCtx;
	int            videoindex, audioindex;
	AVCodecParameters  *m_pVideoCodecParams;
	AVCodecParameters  *m_pAudioCodecParams;
	AVCodec         *m_pVideoCodec;
	AVCodec         *m_pAudioCodec;
	//AVFrame *pFrame, *pFrameRGB,*m_pFrameAudio;
	AVPacket packet;
	int ret;
	char* m_filePath;// = "C:\\¼«ÀÖ¾»ÍÁ 1080p(1).mp4";
	SwsContext* sws_ctx;
	SwrContext* m_swr_ctx;
	AVCodecContext* m_pVideoCodecCtx;
	AVCodecContext* m_pAudioCodecCtx;

	//buffer
	std::queue<AVFrame*> m_VideoBuffer;
	std::queue<AVFrame*> m_AudioBuffer;

private:
	AVFrame* receiveFrame();

public:
	AVReader(char* filePath);
	~AVReader();
	int init();
	AVFrame* receiveFrame(AV_TYPE t);
	void convertVideo(AVFrame* dst, AVFrame* src);
	void convertAudio(AVFrame* dst, AVFrame* src);

	inline int getHeight() {return m_pVideoCodecCtx->height;}
	inline int getWidth(){ return m_pVideoCodecCtx->width; }
};