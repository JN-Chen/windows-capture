// ConsoleApplication4.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>
#include<winsock2.h>
#include <windows.h>


#define __STDC_CONSTANT_MACROS
extern "C"{
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
};

#pragma comment(lib,"avcodec.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib,"avutil.lib")


#pragma comment( lib, "ws2_32.lib" )

char strSaveName[] = "chen.h264";
int capCount = 100000;

int s_W = 800;
int s_H = 600;

#define MAX_FENPIAN 1024
#define PORT 8123
struct HEAD{
	char begin;
	char type;//1,2,3,4
	int count;
	int index;
	int len;
	long frame;
};

char g_SendBuf[1024][MAX_FENPIAN + sizeof(HEAD)];
SOCKET sock;
HANDLE hEvent;//句炳
WSAEVENT hEvent_Update;

int splite(char *buf, int len, int frame){

	int offset = 0;
	int idx = 0;
	HEAD *phead = NULL;
	int count = len%MAX_FENPIAN?(len/MAX_FENPIAN + 1):(len/MAX_FENPIAN);
	int cplen = MAX_FENPIAN;
	while(offset < len){
		phead = (HEAD *)g_SendBuf[idx];
		phead -> index = idx;
		phead -> count = len;
		phead -> frame = frame;
		if(offset + MAX_FENPIAN <= len){
			cplen = MAX_FENPIAN;
			memcpy(g_SendBuf[idx] + sizeof(HEAD), buf + offset, cplen);
			if(offset + cplen == len)phead -> type = 3;
			else phead -> type = 2;
		}
		else{
			cplen = len - offset;
			memcpy(g_SendBuf[idx] + sizeof(HEAD), buf + offset, cplen);
			phead -> type = 3;
		}
		if(idx == 0)phead->begin = 1;
		else phead->begin = 0;

		phead -> len = cplen;
		offset += phead->len;
		idx++;
	}

	return idx;
}

int _tmain(int argc, _TCHAR* argv[])
{

	/*char test[1024*4 + 1];
	for (int i = 0; i < 1024*4 + 1; i++)
	{
		test[i] = i%127;
	}
	splite(test, 1024*4 + 1, 0);*/

	FILE *fp_yuv;
	
	fp_yuv = fopen(strSaveName, "wb+");
	int g_full_width = GetSystemMetrics(SM_CXSCREEN); 
	int g_full_height = GetSystemMetrics(SM_CYSCREEN); 

	HWND hDesktopWnd = ::GetDesktopWindow();
	HDC hDesktopDC = ::GetDC(hDesktopWnd);
	HDC hCaptureDC = ::CreateCompatibleDC(hDesktopDC);
	HBITMAP hCaptureBitmap = ::CreateCompatibleBitmap(hDesktopDC, g_full_width, g_full_height);
	SelectObject(hCaptureDC,hCaptureBitmap);

	long len = 0;
	long size = g_full_width * g_full_height *4;
	char *buf = (char *)malloc(size);

	DWORD tickcount = 0;
	DWORD tickcountn = 0;
	int iframe = 0;
	int iframen = 0;

	int got_output;
	char splitec = ';';
	
    uint8_t *src_data[4], *dst_data[4];
    int src_linesize[4], dst_linesize[4];

	struct SwsContext *sws_ctx;
	AVCodec *codec;
    AVCodecContext *c= NULL;
	AVRational tb;
	tb.den = 25;
	tb.num = 1;
	AVFrame *frame;
    AVPacket pkt;
	
	avcodec_register_all();
	sws_ctx = sws_getContext(g_full_width, g_full_height, AV_PIX_FMT_BGRA,
		s_W, s_H, AV_PIX_FMT_YUV420P,SWS_BICUBIC, NULL, NULL, NULL);
	int ret = av_image_alloc(src_data, src_linesize, g_full_width, g_full_height, AV_PIX_FMT_BGRA, 1);

	ret = av_image_alloc(dst_data, dst_linesize, s_W, s_H, AV_PIX_FMT_YUV420P, 1);

	
    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	
    c = avcodec_alloc_context3(codec);
//	c->bit_rate = 1440;
	c->width = s_W;
    c->height = s_H;
	c->time_base = tb;
	c->gop_size = 10;
    c->max_b_frames = 0;
//	c->qmin = 10;
//	c->qmax = 20;
	/*c->slices = 0;
	c->thread_count = 0;*/
	//c->codec_type = AVMEDIA_TYPE_VIDEO;  
    c->pix_fmt = AV_PIX_FMT_YUV420P;
	
	

//	ret = av_opt_set(c->priv_data, "preset", "ultrafast", 0);
//	ret = av_opt_set(c->priv_data, "tune", "stillimage", 0);
	AVDictionary *opts = NULL;
	//av_dict_set(&opts, "profile", "baseline", 0); 
	 
	avcodec_open2(c, codec, &opts);

	frame = av_frame_alloc();
	frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;
	//ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height,c->pix_fmt, 32);
	frame->linesize[0] = dst_linesize[0];
	frame->data[0] = dst_data[0];
	frame->linesize[1] = dst_linesize[1];
	frame->data[1] = dst_data[1];
	frame->linesize[2] = dst_linesize[2];
	frame->data[2] = dst_data[2];
	int sum = 0;
	long ini =  GetTickCount();


	CURSORINFO tCI;
	ICONINFO tII;

/*	WSADATA wsadata;
	WSAStartup(MAKEWORD(2,2),&wsadata);

	sock = socket(AF_INET,SOCK_DGRAM,0);
	sockaddr_in server;
	server.sin_family=AF_INET;
	server.sin_port=htons(PORT); ///server的监听端口
	server.sin_addr.s_addr=inet_addr("192.168.13.1"); ///server的地址

	hEvent_Update = ::WSACreateEvent();
	::WSAEventSelect(sock, hEvent_Update, FD_READ);*/

	int idx = 0;
	HEAD *phead = NULL;

	while(iframe ++ <= capCount){

		av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;

		tickcountn =  GetTickCount();
		if(tickcountn - tickcount>= 1000){
			tickcount = tickcountn;
			printf("T:%ld, frame:%d\n", tickcount/1000, iframe - iframen);
			iframen = iframe;
		}
		ret = BitBlt(hCaptureDC,0, 0, g_full_width, g_full_height, hDesktopDC, 0, 0, SRCCOPY);

		tCI.cbSize=sizeof(tCI);
		GetCursorInfo(&tCI);
		if(GetIconInfo(tCI.hCursor,&tII))
		{
		}
		DrawIcon(hCaptureDC,tCI.ptScreenPos.x-tII.xHotspot,tCI.ptScreenPos.y-tII.yHotspot,tCI.hCursor);
		if (tII.hbmMask != NULL)
			DeleteObject(tII.hbmMask);
		if (tII.hbmColor != NULL)
			DeleteObject(tII.hbmColor);

		len = GetBitmapBits(hCaptureBitmap, size, src_data[0]);
		

		ret = sws_scale(sws_ctx, (const uint8_t * const*)src_data,src_linesize, 0, g_full_height, dst_data, dst_linesize);
		/*ret = fwrite(dst_data[0], 1, dst_linesize[0] * 600, fp_yuv);
		ret = fwrite(dst_data[1], 1, dst_linesize[1] * 600, fp_yuv);
		ret = fwrite(dst_data[2], 1, dst_linesize[2] * 600, fp_yuv);*/
	//	fflush(fp_yuv);
		/*memcpy(frame->data[0], dst_data[0], frame->linesize[0]);
		memcpy(frame->data[1], dst_data[1], frame->linesize[1]);
		memcpy(frame->data[2], dst_data[2], frame->linesize[2]);*/
	//	frame->pts = iframe/20;
		//ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
		ret = avcodec_send_frame(c, frame);
		ret = avcodec_receive_packet(c, &pkt);
		ret = fwrite(pkt.data, 1, pkt.size, fp_yuv);
		frame->pts++;
	//	ret = fwrite(&splitec, 1, 1, fp_yuv);
		/*
		printf("%x,", pkt.data[0]);
		printf("%x,", pkt.data[1]);
		printf("%x,", pkt.data[2]);
		printf("%x,", pkt.data[3]);
		printf("%x,", pkt.data[4]);
		printf("%x,", pkt.data[5]);
		printf("%x,", pkt.data[6]);
		printf("%x,", pkt.data[7]);
		printf("%x,", pkt.data[8]);
		printf("%x,", pkt.data[9]);
		printf("%x,", pkt.data[10]);*/
	/*	if(pkt.size > 0){
			ret = splite((char *)pkt.data, pkt.size, iframe);
			printf("write len:%d\n", ret);

			for(idx = 0; idx < ret; idx++){
				phead = (HEAD*)g_SendBuf[idx];
				sendto(sock,(char*)g_SendBuf[idx],phead->len + sizeof(HEAD),0,(sockaddr *)&server,len);//发送数据报
				sum += phead->len;
			}
		}*/

		av_packet_unref(&pkt);
		
	}
	printf("write total len:%d\n", sum);
	printf("average iframe:%d\n", iframe*1000/(GetTickCount() - ini));
	getchar();
	fclose(fp_yuv);
	return 0;
}

/*

char *strcat(char *strDest, char *strSrc){
	char *p = strDest;
	while(*strDest!='\0')strDest++;
	while(*strSrc++ != '\0')*strDest = *strSrc;
	return p;
}

void EncString(char *src, char *dst){
	while(*src != '\0'){
		if(*src <= 'a'+13 || *src <= 'A'+ 13)*dst++ = *src + 2*(13 - *src);
		else *dst++ = *src - 2*(*src - 13);
		*src++;
	}
}*/