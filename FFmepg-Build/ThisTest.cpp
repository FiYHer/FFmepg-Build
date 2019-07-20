
#include <stdio.h>
#define __STDC_CONSTANT_MACROS
#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "SDL.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <SDL.h>
#ifdef __cplusplus
};
#endif
#endif
//ffmpeg
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"postproc.lib")
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avfilter.lib")
#pragma comment(lib,"avdevice.lib")
//sdl2
#pragma comment(lib,"x86/SDL2.lib")
#pragma comment(lib,"x86/SDL2main.lib")
#pragma comment(lib,"x86/SDL2test.lib")
#pragma comment(linker, "/subsystem:\"console\" /entry:\"mainCRTStartup\"")

//��ʼ
#define SFM_REFRESH_EVENT (SDL_USEREVENT + 1)
//ֹͣ
#define SFM_BREAK_EVENT (SDL_USEREVENT + 2)

//�߳�ֹͣ
int g_ThreadExit = 0;
//�߳���ͣ
int g_ThreadPause = 0;

//������Ƶ���߳�
int PlayThread(void* pThis)
{
	//��ʼ��ȫ�ֱ���
	g_ThreadExit = 0;
	g_ThreadPause = 0;

	//����߳̽������û�����õĻ�
	while (!g_ThreadExit)
	{
		//����߳�û��ֹͣ�Ļ�
		if (!g_ThreadPause)
		{
			SDL_Event stEvent;
			memset(&stEvent, 0, sizeof(SDL_Event));
			//�׳�һ��ָ�����¼� ����
			stEvent.type = SFM_REFRESH_EVENT;
			SDL_PushEvent(&stEvent);
		}
		//��ͣ40����
		SDL_Delay(40);
	}

	//�ٴ�����
	g_ThreadExit = 0;
	g_ThreadPause = 0;

	//����һ�������̵߳��¼�
	SDL_Event stEvent;
	memset(&stEvent, 0, sizeof(SDL_Event));
	stEvent.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&stEvent);

	return 0;
}

int main(int agrc, char* zrgv[])
{
	//��ʽ��I/O�����Ľṹ��
	//ʹ��avformat_alloc_context���� 
	//http://ffmpeg.org/doxygen/4.1/structAVFormatContext.html
	AVFormatContext* pFormatContext;
	//��������Ƶ����
	int nIndex, nVideoIndex;
	//��Ҫ���ⲿAPI�ṹ
	//����ʹ��AVOptions��av_opt * / av_set / get *���������û�Ӧ�ó��������Щ�ֶ�
	//http://ffmpeg.org/doxygen/4.1/structAVCodecContext.html
	AVCodecContext* pCodeContext;
	//http://ffmpeg.org/doxygen/4.1/structAVCodec.html
	AVCodec* pCode;
	//�ýṹ�����˽���ģ�ԭʼ����Ƶ����Ƶ����
	//http://ffmpeg.org/doxygen/4.1/structAVFrame.html
	AVFrame* pFrame, *pFrameYUV;
	//����ָ��
	unsigned char* uszBuffer;
	//�ýṹ�洢ѹ������
	//http://ffmpeg.org/doxygen/4.1/structAVPacket.html
	AVPacket* pPacket;
	//����ֵ��״ֵ̬
	int nRet, nGotPicture;

	//��Ƶ�Ŀ�Ⱥ͸߶�
	int nScreenW, nScreenH;
	SDL_Window* pWindow;
	SDL_Renderer* pRenderer;
	SDL_Texture* pTexture;
	SDL_Rect stRect;
	SDL_Thread* pThread;
	SDL_Event stEvent;

	//http://ffmpeg.org/doxygen/4.1/structSwsContext.html
	SwsContext* stSwsContext;
	//�ļ�
	char szPath[] = "F:\\ffmepg\\test\\video.mp4";
	//��ʼ������
	av_register_all();
	//��ʼ����ʽ
	avformat_network_init();
	//����һ��FormatContext
	pFormatContext = avformat_alloc_context();
	//�������ļ�
	if (avformat_open_input(&pFormatContext, szPath, 0, 0) != 0)
	{
		puts("open stream file fail");
		return -1;
	}
	//��ȡ�����ļ�����Ϣ
	if (avformat_find_stream_info(pFormatContext, 0) < 0)
	{
		puts("get info fail");
		return -1;
	}

	//������Ƶ֡
	nVideoIndex = -1;
	for (nIndex = 0; nIndex < pFormatContext->nb_streams; nIndex++)
	{
		if (pFormatContext->streams[nIndex]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			nVideoIndex = nIndex;
			break;
		}
	}
	//�����Ƶ֡�Ҳ����Ļ�
	if (nVideoIndex == -1)
	{
		puts("find vidoe fail");
		return -1;
	}
	//��ȡ��������Ϣ
	pCodeContext = pFormatContext->streams[nVideoIndex]->codec;
	//���ұ�����
	pCode = avcodec_find_decoder(pCodeContext->codec_id);
	if (!pCode)
	{
		puts("codec not find");
		return -1;
	}

	if (avcodec_open2(pCodeContext, pCode, 0) < 0)
	{
		puts("could not open codec");
		return -1;
	}

	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();

	uszBuffer = (unsigned char*)malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodeContext->width, pCodeContext->height, 1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, uszBuffer, AV_PIX_FMT_YUV420P, pCodeContext->width, pCodeContext->height, 1);

	puts("----info----");
	av_dump_format(pFormatContext, 0, szPath, 0);
	puts("----ends----");

	stSwsContext = sws_getContext(pCodeContext->width, pCodeContext->height, pCodeContext->pix_fmt, pCodeContext->width, pCodeContext->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, 0, 0, 0);

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		puts("SDL init fail");
		return -1;
	}

	nScreenW = pCodeContext->width;
	nScreenH = pCodeContext->height;

	pWindow = SDL_CreateWindow("This is simply windows", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		nScreenW, nScreenH, SDL_WINDOW_OPENGL);
	if (!pWindow)
	{
		printf("Create SDL Window Fail %s \n",SDL_GetError());
		return -1;
	}

	pRenderer = SDL_CreateRenderer(pWindow, -1, 0);
	if (!pRenderer)
	{
		printf("Create Render Fail %s \n",SDL_GetError());
		return -1;
	}

	pTexture = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
		pCodeContext->width, pCodeContext->height);
	if (!pTexture)
	{
		printf("Create Texture Fail %s \n",SDL_GetError());
		return -1;
	}

	stRect.x = stRect.y = 0;
	stRect.h = nScreenH;
	stRect.w = nScreenW;

	pPacket = (AVPacket*)av_malloc(sizeof(AVPacket));

	pThread = SDL_CreateThread(PlayThread,NULL,NULL);

	while (1)
	{
		SDL_WaitEvent(&stEvent);
		if(stEvent.type == SFM_REFRESH_EVENT)
		{
			while (1)
			{
				if (av_read_frame(pFormatContext, pPacket) < 0)
					g_ThreadExit = 1;
				if(pPacket->stream_index == nVideoIndex)
					break;
			}
			nRet = avcodec_decode_video2(pCodeContext, pFrame, &nGotPicture, pPacket);
			if (nRet < 0)
			{
				puts("Decode Fail");
				return -1;
			}
			if (nGotPicture)
			{
				sws_scale(stSwsContext, (const unsigned char* const*)pFrame->data, pFrame->linesize,0,
					pCodeContext->height, pFrameYUV->data, pFrameYUV->linesize);
				SDL_UpdateTexture(pTexture, 0, pFrameYUV->data[0], pFrameYUV->linesize[0]);
				SDL_RenderClear(pRenderer);
				SDL_RenderCopy(pRenderer, pTexture, 0, 0);
				SDL_RenderPresent(pRenderer);
			}
			av_free_packet(pPacket);
		}
		else if (stEvent.type == SFM_BREAK_EVENT)
		{
			break;
		}
		else if (stEvent.type == SDL_QUIT)
		{
			g_ThreadExit = 1;
		}
		else if (stEvent.type == SDL_KEYDOWN)
		{
			if (stEvent.key.keysym.sym == SDLK_SPACE)
				g_ThreadPause = !g_ThreadPause;	
		}
	}	

	sws_freeContext(stSwsContext);
	SDL_Quit();

	av_frame_free(&pFrame);
	av_frame_free(&pFrameYUV);
	avcodec_close(pCodeContext);
	avformat_close_input(&pFormatContext);
	return 0;
}
