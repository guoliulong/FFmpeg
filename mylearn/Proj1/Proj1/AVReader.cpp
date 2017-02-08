#include "avreader.h"

AVReader::AVReader(char* filePath)
{
	m_filePath = filePath;
}

AVReader::~AVReader()
{
	if (pFrame)
		av_frame_free(&pFrame);
	if (pFrameRGB)
		av_frame_free(&pFrameRGB);
}


bool AVReader::init()
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
	for (i = 0; i<pFormatCtx->nb_streams; i++)
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
		return false;
	}

	if (audioindex == -1)
	{
		printf("Didn't find a audio stream.\n");
		return false;
	}
	//vedio
	m_pVideoCodecParams = pFormatCtx->streams[videoindex]->codecpar;
	m_pVideoCodec = avcodec_find_decoder(m_pVideoCodecParams->codec_id);
	if (m_pVideoCodec == NULL) {
		printf("VCodec not found.\n");
		return false;
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
		return false;
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
		AV_SAMPLE_FMT_S16, 
		441000,
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

	// Allocate video frame
	pFrame = av_frame_alloc();
	pFrameRGB = av_frame_alloc();

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
	int32_t			buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height, 1);
	uint8_t*	buff = (uint8_t *)av_malloc(buffer_size);
	av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buff, AV_PIX_FMT_RGB24, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height, 1);
	// Read frames and save first five frames to disk
	i = 0;
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
			//从解码器返回解码输出数据  
			ret = avcodec_receive_frame(m_pAudioCodecCtx, pFrame);
			if (ret < 0 && ret != AVERROR_EOF)
			{
				if (AVERROR(EAGAIN) == ret)
				{
					printf("AVERROR(EAGAIN)");
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
			swr_convert_frame(m_swr_ctx, pFrame, pFrameRGB);
			pFrameRGB->nb_samples = pFrame->nb_samples;
			return pFrameRGB;
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
			//从解码器返回解码输出数据  
			ret = avcodec_receive_frame(m_pVideoCodecCtx, pFrame);
			if (ret < 0 && ret != AVERROR_EOF)
			{
				if (AVERROR(EAGAIN) == ret)
				{
					printf("AVERROR(EAGAIN)");
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
			sws_scale(sws_ctx, pFrame->data, pFrame->linesize, 0, m_pVideoCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
			pFrameRGB->width = pFrame->width;
			pFrameRGB->height = pFrame->height;
			pFrameRGB->nb_samples = pFrame->nb_samples;
			return pFrameRGB;
		}
	}

	return 0;
}


//
//bool saveBmp(char *bmpName, unsigned char *imgBuf, int width, int height,
//	int biBitCount, RGBQUAD *pColorTable)
//{
//	//如果位图数据指针为0，则没有数据传入，函数返回  
//	if (!imgBuf)
//		return 0;
//	//颜色表大小，以字节为单位，灰度图像颜色表为1024字节，彩色图像颜色表大小为0  
//	int colorTablesize = 0;
//	if (biBitCount == 8)
//		colorTablesize = 1024;
//	//待存储图像数据每行字节数为4的倍数  
//	int lineByte = (width * biBitCount / 8 + 3) / 4 * 4;
//	//以二进制写的方式打开文件  
//	FILE *fp = fopen(bmpName, "wb");
//	if (fp == 0) return 0;
//	//申请位图文件头结构变量，填写文件头信息  
//	BITMAPFILEHEADER fileHead;
//	fileHead.bfType = 0x4D42;//bmp类型  
//							 //bfSize是图像文件4个组成部分之和  
//	fileHead.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)
//		+ colorTablesize + lineByte*height;
//	fileHead.bfReserved1 = 0;
//	fileHead.bfReserved2 = 0;
//	//bfOffBits是图像文件前3个部分所需空间之和  
//	fileHead.bfOffBits = 54 + colorTablesize;
//	//写文件头进文件  
//	fwrite(&fileHead, sizeof(BITMAPFILEHEADER), 1, fp);
//	//申请位图信息头结构变量，填写信息头信息  
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
//	//写位图信息头进内存  
//	fwrite(&head, sizeof(BITMAPINFOHEADER), 1, fp);
//	//如果灰度图像，有颜色表，写入文件    
//	if (biBitCount == 8)
//		fwrite(pColorTable, sizeof(RGBQUAD), 256, fp);
//	//写位图数据进文件  
//	fwrite(imgBuf, height*lineByte, 1, fp);
//	//关闭文件  
//	fclose(fp);
//	return 1;
//}
//
////保存BMP文件的函数  
//void SaveAsBMP(AVFrame *pFrameRGB, int width, int height, int index, int bpp)
//{
//	char buf[5] = { 0 };
//	BITMAPFILEHEADER bmpheader;
//	BITMAPINFOHEADER bmpinfo;
//	FILE *fp;
//
//	char *filename = new char[255];
//
//	//文件存放路径，根据自己的修改  
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