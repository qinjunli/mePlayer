// tutorial03.c
// A pedagogical video player that will stream through every video frame as fast as it can
// and play audio (out of sync).
//
// Code based on FFplay, Copyright (c) 2003 Fabrice Bellard, 
// and a tutorial by Martin Bohme (boehme@inb.uni-luebeckREMOVETHIS.de)
// Tested on Gentoo, CVS version 5/01/07 compiled with GCC 4.1.1
// Use
//
// gcc -o tutorial03 tutorial03.c -lavformat -lavcodec -lz -lm `sdl-config --cflags --libs`
// to build (assuming libavformat and libavcodec are correctly installed, 
// and assuming you have sdl-config. Please refer to SDL docs for your installation.)
//
// Run using
// tutorial03 myvideofile.mpg
//
// to play the stream on your screen.


#include <avcodec.h>
#include <avformat.h>

#include <jni.h>
#include <android/log.h>

#include <SDL.h>
#include <SDL_thread.h>

#ifdef __MINGW32__
#undef main /* Prevents SDL from overriding main() */
#endif

#include <stdio.h>

#define SDL_AUDIO_BUFFER_SIZE 1024

int iWidth,iHeight,Frame_Y_Size;

char file[256];

//play state
#define RUN 0
#define PAUSE 1
#define STOP 2

int state=RUN;


//---------native function  call by Java-----------------------------------
 JNIEXPORT void JNICALL Java_org_libsdl_app_SDLActivity_nativeSetfile
   (JNIEnv *env, jclass cls, jstring filestring){
 	const char *filename=(*env)->GetStringUTFChars(env,filestring,0);
 	strcpy(file,filename);
	__android_log_print(ANDROID_LOG_INFO,"native"," set file completed: %s",file);
 }


 JNIEXPORT void JNICALL Java_org_libsdl_app_SDLActivity_nativePause2
 (JNIEnv *env, jclass cls){
	state=PAUSE;
 }

JNIEXPORT void JNICALL Java_org_libsdl_app_SDLActivity_nativeResume2
 (JNIEnv *env, jclass cls){
	state=RUN;
 }

JNIEXPORT void JNICALL Java_org_libsdl_app_SDLActivity_nativeQuit2
 (JNIEnv *env, jclass cls){
	state=STOP;
 }
//---------------------------------------------------------------------------

typedef struct PacketQueue {
	AVPacketList *first_pkt, *last_pkt;
	int nb_packets;
	int size;
	SDL_mutex *mutex;
	SDL_cond *cond;
} PacketQueue;

PacketQueue audioq;

int quit = 0;

void packet_queue_init(PacketQueue *q) {
	memset(q, 0, sizeof(PacketQueue));
	q->mutex = SDL_CreateMutex();
	q->cond = SDL_CreateCond();
}
int packet_queue_put(PacketQueue *q, AVPacket *pkt) {

	AVPacketList *pkt1;
	if(av_dup_packet(pkt) < 0) {
		return -1;
	}
	pkt1 = av_malloc(sizeof(AVPacketList));
	if (!pkt1)
		return -1;
	pkt1->pkt = *pkt;
	pkt1->next = NULL;


	SDL_LockMutex(q->mutex);

	if (!q->last_pkt)
		q->first_pkt = pkt1;
	else
		q->last_pkt->next = pkt1;
	q->last_pkt = pkt1;
	q->nb_packets++;
	q->size += pkt1->pkt.size;
	SDL_CondSignal(q->cond);

	SDL_UnlockMutex(q->mutex);
	return 0;
}
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block)
{
	AVPacketList *pkt1;
	int ret;

	SDL_LockMutex(q->mutex);

	for(;;) {

		if(quit) {
			ret = -1;
			break;
		}

		pkt1 = q->first_pkt;
		if (pkt1) {
			q->first_pkt = pkt1->next;
			if (!q->first_pkt)
				q->last_pkt = NULL;
			q->nb_packets--;
			q->size -= pkt1->pkt.size;
			*pkt = pkt1->pkt;
			av_free(pkt1);
			ret = 1;
			break;
		} else if (!block) {
			ret = 0;
			break;
		} else {
			SDL_CondWait(q->cond, q->mutex);
		}
	}
	SDL_UnlockMutex(q->mutex);
	return ret;
}

int audio_decode_frame(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size) {

	static AVPacket pkt;
	static uint8_t *audio_pkt_data = NULL;
	static int audio_pkt_size = 0;

	int len1, data_size;

	for(;;) {
		while(audio_pkt_size > 0) {
			data_size = buf_size;
			len1 = avcodec_decode_audio2(aCodecCtx, (int16_t *)audio_buf, &data_size, 
				audio_pkt_data, audio_pkt_size);
			if(len1 < 0) {
				/* if error, skip frame */
				audio_pkt_size = 0;
				break;
			}
			audio_pkt_data += len1;
			audio_pkt_size -= len1;
			if(data_size <= 0) {
				/* No data yet, get more frames */
				continue;
			}
			/* We have data, return it and come back for more later */
			return data_size;
		}
		if(pkt.data)
			av_free_packet(&pkt);

		if(quit) {
			return -1;
		}

		if(packet_queue_get(&audioq, &pkt, 1) < 0) {
			return -1;
		}
		audio_pkt_data = pkt.data;
		audio_pkt_size = pkt.size;
	}
}

void audio_callback(void *userdata, Uint8 *stream, int len) {

	AVCodecContext *aCodecCtx = (AVCodecContext *)userdata;
	int len1, audio_size;

	static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
	static unsigned int audio_buf_size = 0;
	static unsigned int audio_buf_index = 0;

	while(len > 0) {
		if(audio_buf_index >= audio_buf_size) {
			/* We have already sent all our data; get more */
			audio_size = audio_decode_frame(aCodecCtx, audio_buf, sizeof(audio_buf));
			if(audio_size < 0) {
				/* If error, output silence */
				audio_buf_size = 1024; // arbitrary?
				memset(audio_buf, 0, audio_buf_size);
			} else {
				audio_buf_size = audio_size;
			}
			audio_buf_index = 0;
		}
		len1 = audio_buf_size - audio_buf_index;
		if(len1 > len)
			len1 = len;
		memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
		len -= len1;
		stream += len1;
		audio_buf_index += len1;
	}
}

void updateTexture(SDL_Texture *texture,AVFrame *picture)
{
	void *pixels;
    int pitch;
	//Uint8 *dst;
	int i;
	
	if (SDL_LockTexture(texture, NULL, &pixels, &pitch) < 0) {
        fprintf(stderr, "Couldn't lock texture: %s\n", SDL_GetError());
		return;
	}
	
	for(i=0;i<iHeight;i++){
		memcpy((unsigned char*)pixels+i*iWidth,picture->data[0]+i*picture->linesize[0],iWidth);
	}
	for(i=0;i<iHeight/2;i++){
		memcpy((unsigned char*)pixels+Frame_Y_Size+i*iWidth/2,picture->data[2]+i*picture->linesize[2],iWidth/2);
	}
	for(i=0;i<iHeight/2;i++){
		memcpy((unsigned char*)pixels+Frame_Y_Size*5/4+i*iWidth/2,picture->data[1]+i*picture->linesize[1],iWidth/2);
	}
	/*
	memcpy(pixels,picture->data[0],Frame_Y_Size);
	memcpy(pixels+Frame_Y_Size,picture->data[2],Frame_Y_Size/4);
	memcpy(pixels+Frame_Y_Size*5/4,picture->data[1],Frame_Y_Size/4);
	*/
	SDL_UnlockTexture(texture);
}

int main(int argc, char *argv[]) {
	AVFormatContext *pFormatCtx;
	int             i, videoStream, audioStream;
	AVCodecContext  *pCodecCtx;
	AVCodec         *pCodec;
	AVFrame         *pFrame; 
	AVPacket        packet;
	int             frameFinished;
	float           aspect_ratio;

	AVCodecContext  *aCodecCtx;
	AVCodec         *aCodec;

	//SDL_Overlay     *bmp;
	//SDL_Surface     *screen;
	//SDL_Rect        rect;
	SDL_Window		*pWindow;
	SDL_Renderer	*pRenderer;
	SDL_Texture		*streamText;
	SDL_Event       event;
	SDL_AudioSpec   wanted_spec, spec;
	/*
	if(argc < 2) {
		fprintf(stderr, "Usage: test <file>\n");
		exit(1);
	}
	*/
	// Register all formats and codecs
	av_register_all();

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}

	// Open video file
	if(av_open_input_file(&pFormatCtx, file, NULL, 0, NULL)!=0)
	{
		__android_log_print(ANDROID_LOG_ERROR,"native"," Couldn't open file: %s",file);
		return -1; // Couldn't open file
	}

	// Retrieve stream information
	if(av_find_stream_info(pFormatCtx)<0)
	{
		return -1; // Couldn't find stream information
		__android_log_print(ANDROID_LOG_ERROR,"native"," Couldn't find stream information: %s",file);
	}

	// Dump information about file onto standard error
	dump_format(pFormatCtx, 0, argv[1], 0);

	// Find the first video stream
	videoStream=-1;
	audioStream=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++) {
		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO &&
			videoStream < 0) {
				videoStream=i;
		}
		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_AUDIO &&
			audioStream < 0) {
				audioStream=i;
		}
	}
	if(videoStream==-1)
	{
		return -1; // Didn't find a video stream
		__android_log_print(ANDROID_LOG_ERROR,"native"," Didn't find a video stream: %s",file);
	}
	if(audioStream==-1)
	{
		return -1;
		__android_log_print(ANDROID_LOG_ERROR,"native"," Didn't find a audio stream: %s",file);
	}

	aCodecCtx=pFormatCtx->streams[audioStream]->codec;
	// Set audio settings from codec info
	wanted_spec.freq = aCodecCtx->sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = aCodecCtx->channels;
	wanted_spec.silence = 0;
	wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
	wanted_spec.callback = audio_callback;
	wanted_spec.userdata = aCodecCtx;

	if(SDL_OpenAudio(&wanted_spec, &spec) < 0) {
		//fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
		__android_log_print(ANDROID_LOG_ERROR,"native"," SDL_OpenAudio: %s",SDL_GetError());
		return -1;
	}
	aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
	if(!aCodec) {
		//fprintf(stderr, "Unsupported codec!\n");
		__android_log_print(ANDROID_LOG_ERROR,"native"," Unsupported codec");
		return -1;
	}
	avcodec_open(aCodecCtx, aCodec);

	// audio_st = pFormatCtx->streams[index]
	packet_queue_init(&audioq);
	SDL_PauseAudio(0);

	// Get a pointer to the codec context for the video stream
	pCodecCtx=pFormatCtx->streams[videoStream]->codec;
	iWidth=pCodecCtx->width;
	iHeight=pCodecCtx->height;
	Frame_Y_Size=iWidth*iHeight;

	// Find the decoder for the video stream
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL) {
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}
	// Open codec
	if(avcodec_open(pCodecCtx, pCodec)<0)
		return -1; // Could not open codec

	// Allocate video frame
	pFrame=avcodec_alloc_frame();


	//SDL 2.0 new Struct allocate
	pWindow=SDL_CreateWindow(argv[1],SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,
		pCodecCtx->width,pCodecCtx->height,SDL_WINDOW_SHOWN);
	if (pWindow==NULL)
	{
		fprintf(stderr,"cannot create window : %s\n",SDL_GetError());
		return -1;
	}
	pRenderer=SDL_CreateRenderer(pWindow,-1,SDL_RENDERER_ACCELERATED);
	if (pRenderer==NULL)
	{
		fprintf(stderr,"can not create renderer : %s\n",SDL_GetError());
		return -1;
	}
	streamText=SDL_CreateTexture(pRenderer,SDL_PIXELFORMAT_YV12,
		SDL_TEXTUREACCESS_STREAMING,pCodecCtx->width,pCodecCtx->height);
	if (streamText==NULL)
	{
		fprintf(stderr,"cannot create Texture : %s\n",SDL_GetError());
		return -1;
	}
	
	

	// Make a screen to put our video
	/*

#ifndef __DARWIN__
	screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);
#else
	screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 24, 0);
#endif
	if(!screen) {
		fprintf(stderr, "SDL: could not set video mode - exiting\n");
		exit(1);
	}

	// Allocate a place to put our YUV image on that screen
	bmp = SDL_CreateYUVOverlay(pCodecCtx->width,
		pCodecCtx->height,
		SDL_YV12_OVERLAY,
		screen);
	*/
	

	// Read frames and save first five frames to disk
	i=0;
	while(av_read_frame(pFormatCtx, &packet)>=0) {
		// Is this a packet from the video stream?
		if(packet.stream_index==videoStream) {
			// Decode video frame
			avcodec_decode_video(pCodecCtx, pFrame, &frameFinished, 
				packet.data, packet.size);

			// Did we get a video frame?
			if(frameFinished) {
				updateTexture(streamText,pFrame);
				SDL_RenderClear(pRenderer);

				SDL_RenderCopy(pRenderer, streamText, NULL, NULL);

				SDL_RenderPresent(pRenderer);
				av_free_packet(&packet);
			}
		} else if(packet.stream_index==audioStream) {
			packet_queue_put(&audioq, &packet);
		} else {
			av_free_packet(&packet);
		}
		// Free the packet that was allocated by av_read_frame

		while(state==PAUSE){
			SDL_Delay(200);
		}
		if(state==STOP)
		{
			break;
		}

		/*
		SDL_PollEvent(&event);
		switch(event.type) {
	case SDL_QUIT:
		quit = 1;
		SDL_Quit();
		exit(0);
		break;
	default:
		break;
		}
		*/

	}

	// Free the YUV frame
	av_free(pFrame);

	// Close the codec
	avcodec_close(pCodecCtx);

	// Close the video file
	av_close_input_file(pFormatCtx);

	if(state==STOP)
	{
		SDL_DestroyTexture(streamText);
		SDL_DestroyRenderer(pRenderer);
		SDL_DestroyWindow(pWindow);
		SDL_Quit();
		exit(0);
	}

	return 0;
}
