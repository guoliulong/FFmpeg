// ConsoleApplication2.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include<Windows.h>
extern "C"
{
#include "libavcodec/avcodec.h"  
#include "libavformat/avformat.h"  
#include "libswscale/swscale.h"  
#include "libavutil/imgutils.h"
};

extern bool saveBmp(char *bmpName, unsigned char *imgBuf, int width, int height,
	int biBitCount, RGBQUAD *pColorTable);
void SaveAsBMP(AVFrame *pFrameRGB, int width, int height, int index, int bpp);

int main()
{
	//FFmpeg  
	AVFormatContext *pFormatCtx;
	int             i, videoindex;
	AVCodecParameters  *pCodecParams;
	AVCodec         *pCodec;
	AVFrame *pFrame, *pFrameRGB;
	AVPacket packet;
	struct SwsContext *img_convert_ctx;
	//SDL  
	int screen_w, screen_h;
	/*SDL_Surface *screen;
	SDL_VideoInfo *vi;
	SDL_Overlay *bmp;
	SDL_Rect rect;*/

	FILE *fp_yuv;
	int ret, got_picture;
	//char filepath[] = "C:\\���־��� 1080p(1).mp4";
	char filepath[] = "G:\\EP18.mp4";

	av_register_all();
	avformat_network_init();

	pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0)
	{
		printf("Couldn't open input stream.\n");
		return -1;
	}

	if (avformat_find_stream_info(pFormatCtx, NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
	av_dump_format(pFormatCtx, 0, filepath, 0);

	videoindex = -1;
	for (i = 0; i<pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}

	if (videoindex == -1) {
		printf("Didn't find a video stream.\n");
		return -1;
	}

	pCodecParams = pFormatCtx->streams[videoindex]->codecpar;
	pCodec = avcodec_find_decoder(pCodecParams->codec_id);
	if (pCodec == NULL) {
		printf("Codec not found.\n");
		return -1;
	}


	AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
	if (avcodec_parameters_to_context(pCodecCtx, pCodecParams) != 0)
	{
		return -1;
	}

	if (avcodec_open2(pCodecCtx, pCodec, NULL)<0) {
		printf("Could not open codec.\n");
		return -1;
	}

	// Allocate video frame
	pFrame = av_frame_alloc();
	pFrameRGB = av_frame_alloc();

	// initialize SWS context for software scaling
	SwsContext* sws_ctx = sws_getContext(pCodecCtx->width,
		pCodecCtx->height,
		pCodecCtx->pix_fmt,
		pCodecCtx->width,
		pCodecCtx->height,
		AV_PIX_FMT_RGB24,
		SWS_SINC,
		NULL,
		NULL,
		NULL
	);

	// Read frames and save first five frames to disk
	i = 0;
	while (av_read_frame(pFormatCtx, &packet) >= 0)
	{
		// Is this a packet from the video stream?
		if (packet.stream_index == videoindex)
		{
			int frameFinished;

			ret = avcodec_send_packet(pCodecCtx, &packet);
			if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
			{
				av_packet_unref(&packet);
				return -1;
			}
			//�ӽ��������ؽ����������  
			ret = avcodec_receive_frame(pCodecCtx, pFrame);
			if (ret < 0 && ret != AVERROR_EOF)
			{
				if (AVERROR(EAGAIN) == ret)
				{
					printf("AVERROR(EAGAIN)");
					continue;
				}

				av_packet_unref(&packet);
				return -1;
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

			static int count = 0;
			count++;

			if (ret == 0)
			{
				//printf("%d\n", count);
			}

			int			buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);
			uint8_t*	buff = (uint8_t *)av_malloc(buffer_size);
			av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buff, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);

			sws_scale(sws_ctx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

			//SaveAsBMP(pFrameRGB, pCodecCtx->width, pCodecCtx->height, 0, 24);
			
			uint8_t*	buff32 = (uint8_t *)av_malloc(buffer_size/3*4);

			for (int y = 0; y < pCodecCtx->height; y++)
			{
				for (int x = 0; x < pCodecCtx->width; x++)
				{
					int index = (pCodecCtx->width * y + x) * 3;
					int index32 = (pCodecCtx->width * y + x) * 4;
					uint8_t* ppixel = pFrameRGB->data[0];

					buff32[index32+2] = ppixel[index];
					buff32[index32+1] = ppixel[index+1];
					buff32[index32] = ppixel[index+2];
					//buff32[index32 + 3] = 255;
				}
			}


			HWND hwnd =		::GetDesktopWindow();
			HDC hdc =		::GetDC(hwnd);
			HDC memDc =		::CreateCompatibleDC(hdc);
			HBRUSH brush =	::CreateSolidBrush(RGB(0, 255, 0));
			HBITMAP bitmap = ::CreateBitmap(pCodecCtx->width, pCodecCtx->height, 1, 32, buff32);
			::SelectObject(memDc, bitmap);
			

			::StretchBlt(hdc, 0, 0, pCodecCtx->width, pCodecCtx->height, memDc, 0, 0, pCodecCtx->width, pCodecCtx->height, SRCCOPY);

			::DeleteObject(brush);
			::DeleteObject(bitmap);
			::DeleteObject(memDc);

			av_free(buff);
			av_free(buff32);
		}
	}



	return 0;
}


bool saveBmp(char *bmpName, unsigned char *imgBuf, int width, int height,
	int biBitCount, RGBQUAD *pColorTable)
{
	//���λͼ����ָ��Ϊ0����û�����ݴ��룬��������  
	if (!imgBuf)
		return 0;
	//��ɫ���С�����ֽ�Ϊ��λ���Ҷ�ͼ����ɫ��Ϊ1024�ֽڣ���ɫͼ����ɫ���СΪ0  
	int colorTablesize = 0;
	if (biBitCount == 8)
		colorTablesize = 1024;
	//���洢ͼ������ÿ���ֽ���Ϊ4�ı���  
	int lineByte = (width * biBitCount / 8 + 3) / 4 * 4;
	//�Զ�����д�ķ�ʽ���ļ�  
	FILE *fp = fopen(bmpName, "wb");
	if (fp == 0) return 0;
	//����λͼ�ļ�ͷ�ṹ��������д�ļ�ͷ��Ϣ  
	BITMAPFILEHEADER fileHead;
	fileHead.bfType = 0x4D42;//bmp����  
							 //bfSize��ͼ���ļ�4����ɲ���֮��  
	fileHead.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)
		+ colorTablesize + lineByte*height;
	fileHead.bfReserved1 = 0;
	fileHead.bfReserved2 = 0;
	//bfOffBits��ͼ���ļ�ǰ3����������ռ�֮��  
	fileHead.bfOffBits = 54 + colorTablesize;
	//д�ļ�ͷ���ļ�  
	fwrite(&fileHead, sizeof(BITMAPFILEHEADER), 1, fp);
	//����λͼ��Ϣͷ�ṹ��������д��Ϣͷ��Ϣ  
	BITMAPINFOHEADER head;
	head.biBitCount = biBitCount;
	head.biClrImportant = 0;
	head.biClrUsed = 0;
	head.biCompression = 0;
	head.biHeight = height;
	head.biPlanes = 1;
	head.biSize = 40;
	head.biSizeImage = lineByte*height;
	head.biWidth = width;
	head.biXPelsPerMeter = 0;
	head.biYPelsPerMeter = 0;
	//дλͼ��Ϣͷ���ڴ�  
	fwrite(&head, sizeof(BITMAPINFOHEADER), 1, fp);
	//����Ҷ�ͼ������ɫ��д���ļ�    
	if (biBitCount == 8)
		fwrite(pColorTable, sizeof(RGBQUAD), 256, fp);
	//дλͼ���ݽ��ļ�  
	fwrite(imgBuf, height*lineByte, 1, fp);
	//�ر��ļ�  
	fclose(fp);
	return 1;
}

//����BMP�ļ��ĺ���  
void SaveAsBMP(AVFrame *pFrameRGB, int width, int height, int index, int bpp)
{
	char buf[5] = { 0 };
	BITMAPFILEHEADER bmpheader;
	BITMAPINFOHEADER bmpinfo;
	FILE *fp;

	char *filename = new char[255];

	//�ļ����·���������Լ����޸�  
	sprintf_s(filename, 255, "%s%d.bmp", "X", index);
	if ((fp = fopen(filename, "wb+")) == NULL) {
		printf("open file failed!\n");
		return;
	}

	bmpheader.bfType = 0x4d42;
	bmpheader.bfReserved1 = 0;
	bmpheader.bfReserved2 = 0;
	bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmpheader.bfSize = bmpheader.bfOffBits + width*height*bpp / 8;

	bmpinfo.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfo.biWidth = width;
	bmpinfo.biHeight = height;
	bmpinfo.biPlanes = 1;
	bmpinfo.biBitCount = bpp;
	bmpinfo.biCompression = BI_RGB;
	bmpinfo.biSizeImage = (width*bpp + 31) / 32 * 4 * height;
	bmpinfo.biXPelsPerMeter = 100;
	bmpinfo.biYPelsPerMeter = 100;
	bmpinfo.biClrUsed = 0;
	bmpinfo.biClrImportant = 0;

	fwrite(&bmpheader, sizeof(bmpheader), 1, fp);
	fwrite(&bmpinfo, sizeof(bmpinfo), 1, fp);
	fwrite(pFrameRGB->data[0], width*height*bpp / 8, 1, fp);
	fflush(fp);
	fclose(fp);
}