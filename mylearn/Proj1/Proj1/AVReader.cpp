#include "avreader.h"
#include <assert.h>

static char err_buf[128] = { 0 };
static char* av_get_err(int errnum)
{
	av_strerror(errnum, err_buf, 128);
	return err_buf;
}


AVReader::AVReader(const char* filePath)
{
	m_filePath = filePath;
}

AVReader::~AVReader()
{
	/*if (pFrame)
		av_frame_free(&pFrame);
	if (pFrameRGB)
		av_frame_free(&pFrameRGB);*/
}


int AVReader::init()
{
	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&pFormatCtx, m_filePath, NULL, NULL) != 0)
	{
		printf("Couldn't open input stream.\n");
		return -1;
	}

	if (avformat_find_stream_info(pFormatCtx, NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
	av_dump_format(pFormatCtx, 0, m_filePath, 0);

	videoindex = -1;
	audioindex = -1;
	for (uint32_t i = 0; i<pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
		}
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioindex = i;
		}
	}
	if (videoindex == -1) 
	{
		printf("Didn't find a video stream.\n");
		return -1;
	}

	if (audioindex == -1)
	{
		printf("Didn't find a audio stream.\n");
		return -1;
	}
	//vedio
	m_pVideoCodecParams = pFormatCtx->streams[videoindex]->codecpar;
	m_pVideoCodec = avcodec_find_decoder(m_pVideoCodecParams->codec_id);
	if (m_pVideoCodec == NULL) {
		printf("VCodec not found.\n");
		return -1;
	}
	m_pVideoCodecCtx = avcodec_alloc_context3(m_pVideoCodec);
	if (avcodec_parameters_to_context(m_pVideoCodecCtx, m_pVideoCodecParams) != 0)
	{
		return -1;
	}
	if (avcodec_open2(m_pVideoCodecCtx, m_pVideoCodec, NULL)<0) {
		printf("Could not open codec.\n");
		return -1;
	}

	//audio
	m_pAudioCodecParams = pFormatCtx->streams[audioindex]->codecpar;
	m_pAudioCodec = avcodec_find_decoder(m_pAudioCodecParams->codec_id);
	if (m_pAudioCodec == NULL) {
		printf("ACodec not found.\n");
		return -1;
	}
	m_pAudioCodecCtx = avcodec_alloc_context3(m_pAudioCodec);
	if (avcodec_parameters_to_context(m_pAudioCodecCtx, m_pAudioCodecParams) != 0)
	{
		return -1;
	}
	if (avcodec_open2(m_pAudioCodecCtx, m_pAudioCodec, nullptr)<0) {
		printf("Could not open codec.\n");
		return -1;
	}

	m_swr_ctx = swr_alloc_set_opts(
		NULL,
		AV_CH_LAYOUT_STEREO,
		AV_SAMPLE_FMT_U8,
		44100,
		m_pAudioCodecParams->channel_layout,
		(AVSampleFormat)m_pAudioCodecParams->format,
		m_pAudioCodecParams->sample_rate,
		0,
		0);


	if (!m_swr_ctx || swr_init(m_swr_ctx) < 0)
	{
		printf("swr error .\n");
		return -1;
	}

	//m_pFrameAudio = av_frame_alloc();
	//m_pFrameAudio->format = AV_SAMPLE_FMT_U8;
	//m_pFrameAudio->sample_rate = 44100;
	//m_pFrameAudio->channel_layout = AV_CH_LAYOUT_STEREO;

	//// Allocate video frame
	//pFrame = av_frame_alloc();
	//pFrameRGB = av_frame_alloc();

	// initialize SWS context for software scaling
	sws_ctx = sws_getContext(m_pVideoCodecCtx->width,
		m_pVideoCodecCtx->height,
		m_pVideoCodecCtx->pix_fmt,
		m_pVideoCodecCtx->width,
		m_pVideoCodecCtx->height,
		AV_PIX_FMT_RGB24,
		SWS_SINC,
		NULL,
		NULL,
		NULL
	);
	//int32_t			buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height, 1);
	//uint8_t*	buff = (uint8_t *)av_malloc(buffer_size);
	//av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buff, AV_PIX_FMT_RGB24, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height, 1);
	// Read frames and save first five frames to disk

	return 0;
}

AVFrame * AVReader::receiveFrame(AV_TYPE t)
{
	switch (t)
	{
	case AT_AUDIO:
	{
		if (!m_pAudioCodecCtx)
			return nullptr;

		if (m_AudioBuffer.size() > 0)
		{
			AVFrame * ret = m_AudioBuffer.back();
			m_AudioBuffer.pop();
			return ret;
		}

		AVFrame* avf = receiveFrame();
		while(avf->nb_samples == 0)
		{
			m_VideoBuffer.push(av_frame_clone(avf));
			avf = receiveFrame();
		}
			
		return avf;
		
	}
	break;
	case AT_VIDEO:
		if (!m_pVideoCodecCtx)
			return nullptr;

		if (m_VideoBuffer.size() > 0)
		{
			AVFrame * ret = m_VideoBuffer.back();
			m_VideoBuffer.pop();
			return ret;
		}
		AVFrame* avf = receiveFrame();
		while (avf->nb_samples != 0)
		{
			m_AudioBuffer.push(av_frame_clone(avf));
			avf = receiveFrame();
		}
		return avf;
		
		break;
	}

	return nullptr;
}

void AVReader::convertVideo(AVFrame * dst, AVFrame * src)
{
	sws_scale(sws_ctx, src->data, src->linesize, 0, m_pVideoCodecCtx->height, dst->data, dst->linesize);
	dst->width = src->width;
	dst->height = src->height;
	dst->nb_samples = src->nb_samples;
}

void AVReader::convertAudio(AVFrame * dst, AVFrame * src)
{
	int ret = swr_get_out_samples(m_swr_ctx, src->nb_samples);
	// Input and output AVFrames must have channel_layout, sample_rate and format set.
	dst->sample_rate = 44100;
	dst->channel_layout = AV_CH_LAYOUT_STEREO;
	dst->format = AV_SAMPLE_FMT_U8;
	dst->channels = 2;

	ret *= 2;

	if (ret > dst->pkt_size)
	{
		if (dst->data[0])
		{
			dst->data[0] = (uint8_t*)realloc(dst->data[0], ret);
		}
		else
		{
			dst->data[0] = (uint8_t*)malloc(ret);
		}
		dst->pkt_size = ret;
	}

	if (ret=swr_convert_frame(m_swr_ctx, dst, src) != 0)
	{
		printf("swr_convert_frame failed %s", av_get_err(ret));
	}
}

AVFrame* AVReader::receiveFrame()
{
	while (av_read_frame(pFormatCtx, &packet) >= 0)
	{
		// Is this a packet from the audio stream?
		if (packet.stream_index == audioindex)
		{
			ret = avcodec_send_packet(m_pAudioCodecCtx, &packet);
			if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
			{
				av_packet_unref(&packet);
				return 0;
			}
			//�ӽ��������ؽ����������
			AVFrame* newframe = av_frame_alloc();
			ret = avcodec_receive_frame(m_pAudioCodecCtx, newframe);
			if (ret < 0 && ret != AVERROR_EOF)
			{
				if (AVERROR(EAGAIN) == ret)
				{
					av_frame_free(&newframe);
					continue;
				}
				av_packet_unref(&packet);
				return 0;
			}

			switch (ret)
			{
			case AVERROR(EAGAIN):
				printf("AAVERROR(EAGAIN)");
				break;
			case AVERROR_EOF:
				printf("AAVERROR_EOF");
				break;
			case AVERROR(EINVAL):
				printf("AAVERROR(EINVAL)");
				break;
			case 0:
				break;
			}

			return newframe;
		}
		//video
		if (packet.stream_index == videoindex)
		{
			ret = avcodec_send_packet(m_pVideoCodecCtx, &packet);
			if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
			{
				av_packet_unref(&packet);
				return 0;
			}
			//�ӽ��������ؽ����������  
			AVFrame* newframe = av_frame_alloc();
			ret = avcodec_receive_frame(m_pVideoCodecCtx, newframe);
			if (ret < 0 && ret != AVERROR_EOF)
			{
				if (AVERROR(EAGAIN) == ret)
				{
					av_frame_free(&newframe);
					continue;
				}

				av_packet_unref(&packet);
				return 0;
			}

			switch (ret)
			{
			case AVERROR(EAGAIN):
				printf("AVERROR(EAGAIN)");
				break;
			case AVERROR_EOF:
				printf("AVERROR_EOF");
				break;
			case AVERROR(EINVAL):
				printf("AVERROR(EINVAL)");
				break;
			case 0:
				break;
			}
			return newframe;
		}
	}

	return 0;
}


//
//bool saveBmp(char *bmpName, unsigned char *imgBuf, int width, int height,
//	int biBitCount, RGBQUAD *pColorTable)
//{
//	//���λͼ����ָ��Ϊ0����û�����ݴ��룬��������  
//	if (!imgBuf)
//		return 0;
//	//��ɫ���С�����ֽ�Ϊ��λ���Ҷ�ͼ����ɫ��Ϊ1024�ֽڣ���ɫͼ����ɫ���СΪ0  
//	int colorTablesize = 0;
//	if (biBitCount == 8)
//		colorTablesize = 1024;
//	//���洢ͼ������ÿ���ֽ���Ϊ4�ı���  
//	int lineByte = (width * biBitCount / 8 + 3) / 4 * 4;
//	//�Զ�����д�ķ�ʽ���ļ�  
//	FILE *fp = fopen(bmpName, "wb");
//	if (fp == 0) return 0;
//	//����λͼ�ļ�ͷ�ṹ��������д�ļ�ͷ��Ϣ  
//	BITMAPFILEHEADER fileHead;
//	fileHead.bfType = 0x4D42;//bmp����  
//							 //bfSize��ͼ���ļ�4����ɲ���֮��  
//	fileHead.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)
//		+ colorTablesize + lineByte*height;
//	fileHead.bfReserved1 = 0;
//	fileHead.bfReserved2 = 0;
//	//bfOffBits��ͼ���ļ�ǰ3����������ռ�֮��  
//	fileHead.bfOffBits = 54 + colorTablesize;
//	//д�ļ�ͷ���ļ�  
//	fwrite(&fileHead, sizeof(BITMAPFILEHEADER), 1, fp);
//	//����λͼ��Ϣͷ�ṹ��������д��Ϣͷ��Ϣ  
//	BITMAPINFOHEADER head;
//	head.biBitCount = biBitCount;
//	head.biClrImportant = 0;
//	head.biClrUsed = 0;
//	head.biCompression = 0;
//	head.biHeight = height;
//	head.biPlanes = 1;
//	head.biSize = 40;
//	head.biSizeImage = lineByte*height;
//	head.biWidth = width;
//	head.biXPelsPerMeter = 0;
//	head.biYPelsPerMeter = 0;
//	//дλͼ��Ϣͷ���ڴ�  
//	fwrite(&head, sizeof(BITMAPINFOHEADER), 1, fp);
//	//����Ҷ�ͼ������ɫ��д���ļ�    
//	if (biBitCount == 8)
//		fwrite(pColorTable, sizeof(RGBQUAD), 256, fp);
//	//дλͼ���ݽ��ļ�  
//	fwrite(imgBuf, height*lineByte, 1, fp);
//	//�ر��ļ�  
//	fclose(fp);
//	return 1;
//}
//
////����BMP�ļ��ĺ���  
//void SaveAsBMP(AVFrame *pFrameRGB, int width, int height, int index, int bpp)
//{
//	char buf[5] = { 0 };
//	BITMAPFILEHEADER bmpheader;
//	BITMAPINFOHEADER bmpinfo;
//	FILE *fp;
//
//	char *filename = new char[255];
//
//	//�ļ����·���������Լ����޸�  
//	sprintf_s(filename, 255, "%s%d.bmp", "X", index);
//	if ((fp = fopen(filename, "wb+")) == NULL) {
//		printf("open file failed!\n");
//		return;
//	}
//
//	bmpheader.bfType = 0x4d42;
//	bmpheader.bfReserved1 = 0;
//	bmpheader.bfReserved2 = 0;
//	bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
//	bmpheader.bfSize = bmpheader.bfOffBits + width*height*bpp / 8;
//
//	bmpinfo.biSize = sizeof(BITMAPINFOHEADER);
//	bmpinfo.biWidth = width;
//	bmpinfo.biHeight = height;
//	bmpinfo.biPlanes = 1;
//	bmpinfo.biBitCount = bpp;
//	bmpinfo.biCompression = BI_RGB;
//	bmpinfo.biSizeImage = (width*bpp + 31) / 32 * 4 * height;
//	bmpinfo.biXPelsPerMeter = 100;
//	bmpinfo.biYPelsPerMeter = 100;
//	bmpinfo.biClrUsed = 0;
//	bmpinfo.biClrImportant = 0;
//
//	fwrite(&bmpheader, sizeof(bmpheader), 1, fp);
//	fwrite(&bmpinfo, sizeof(bmpinfo), 1, fp);
//	fwrite(pFrameRGB->data[0], width*height*bpp / 8, 1, fp);
//	fflush(fp);
//	fclose(fp);
//}