
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

//开始
#define SFM_REFRESH_EVENT (SDL_USEREVENT + 1)
//停止
#define SFM_BREAK_EVENT (SDL_USEREVENT + 2)

//线程停止
int g_ThreadExit = 0;
//线程暂停
int g_ThreadPause = 0;

//播放视频的线程
int PlayThread(void* pThis)
{
	//初始化全局变量
	g_ThreadExit = 0;
	g_ThreadPause = 0;

	//如果线程结束标记没有设置的话
	while (!g_ThreadExit)
	{
		//如果线程没有停止的话
		if (!g_ThreadPause)
		{
			SDL_Event stEvent;
			memset(&stEvent, 0, sizeof(SDL_Event));
			//抛出一个指定的事件 播放
			stEvent.type = SFM_REFRESH_EVENT;
			SDL_PushEvent(&stEvent);
		}
		//暂停40毫秒
		SDL_Delay(40);
	}

	//再次设置
	g_ThreadExit = 0;
	g_ThreadPause = 0;

	//发送一个结束线程的事件
	SDL_Event stEvent;
	memset(&stEvent, 0, sizeof(SDL_Event));
	stEvent.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&stEvent);

	return 0;
}

int main(int agrc, char* zrgv[])
{
	//格式化I/O上下文结构体
	//使用avformat_alloc_context（） 
	//http://ffmpeg.org/doxygen/4.1/structAVFormatContext.html
	AVFormatContext* pFormatContext;
	//索引和视频索引
	int nIndex, nVideoIndex;
	//主要的外部API结构
	//可以使用AVOptions（av_opt * / av_set / get *（））从用户应用程序访问这些字段
	//http://ffmpeg.org/doxygen/4.1/structAVCodecContext.html
	AVCodecContext* pCodeContext;
	//http://ffmpeg.org/doxygen/4.1/structAVCodec.html
	AVCodec* pCode;
	//该结构描述了解码的（原始）音频或视频数据
	//http://ffmpeg.org/doxygen/4.1/structAVFrame.html
	AVFrame* pFrame, *pFrameYUV;
	//数据指针
	unsigned char* uszBuffer;
	//该结构存储压缩数据
	//http://ffmpeg.org/doxygen/4.1/structAVPacket.html
	AVPacket* pPacket;
	//返回值和状态值
	int nRet, nGotPicture;

	//视频的宽度和高度
	int nScreenW, nScreenH;
	SDL_Window* pWindow;
	SDL_Renderer* pRenderer;
	SDL_Texture* pTexture;
	SDL_Rect stRect;
	SDL_Thread* pThread;
	SDL_Event stEvent;

	//http://ffmpeg.org/doxygen/4.1/structSwsContext.html
	SwsContext* stSwsContext;
	//文件
	char szPath[] = "F:\\ffmepg\\test\\video.mp4";
	//初始化所有
	av_register_all();
	//初始化格式
	avformat_network_init();
	//申请一个FormatContext
	pFormatContext = avformat_alloc_context();
	//打开输入文件
	if (avformat_open_input(&pFormatContext, szPath, 0, 0) != 0)
	{
		puts("open stream file fail");
		return -1;
	}
	//获取输入文件的信息
	if (avformat_find_stream_info(pFormatContext, 0) < 0)
	{
		puts("get info fail");
		return -1;
	}

	//查找视频帧
	nVideoIndex = -1;
	for (nIndex = 0; nIndex < pFormatContext->nb_streams; nIndex++)
	{
		if (pFormatContext->streams[nIndex]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			nVideoIndex = nIndex;
			break;
		}
	}
	//如果视频帧找不到的话
	if (nVideoIndex == -1)
	{
		puts("find vidoe fail");
		return -1;
	}
	//获取编码器信息
	pCodeContext = pFormatContext->streams[nVideoIndex]->codec;
	//查找编码器
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
