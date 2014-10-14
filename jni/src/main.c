
#include <avcodec.h>
#include <avformat.h>
#include <jni.h>

#include <SDL.h>
#include <SDL_thread.h>
#include <android/log.h>
#include <time.h>
#include <stdlib.h>

#ifdef __MINGW32__
#undef main /* Prevents SDL from overriding main() */
#endif

#include <stdio.h>

#define RUN 0
#define PAUSE 1
#define STOP 2

//jobject surfaceObj=NULL;
//JNIEnv *globalEnv=NULL;

int iHeight,iWidth,Frame_Y_Size;
int whKnown_flag=0;

unsigned char mu;

unsigned char *Y_data,*U_data,*V_data;//存储经过补偿后的Y、U、V分量

/*---亮度---*/
unsigned char backlight;
unsigned char backlightOri;
int energy_flag=0;
char file[256];
int state=RUN;
//float alpha_gamma_table[256];

JNIEXPORT void JNICALL Java_org_libsdl_app_SDLActivity_nativeSetfile
  (JNIEnv *env, jclass cls, jstring filestring){
	const char *filename=(*env)->GetStringUTFChars(env,filestring,0);
	strcpy(file,filename);
  }
  JNIEXPORT void JNICALL Java_org_libsdl_app_SDLSurface_native_1energy_1switch
  (JNIEnv *env, jobject obj){
	energy_flag=(energy_flag+1)%3;
  }
  
  JNIEXPORT void JNICALL Java_org_libsdl_app_SDLSurface_setBl
  (JNIEnv *env, jobject obj, jint java_bl)
  {
	  backlightOri=java_bl;
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
  
  JNIEXPORT jint JNICALL Java_org_libsdl_app_SDLSurface_getBl
  (JNIEnv *env, jobject obj)
  {
	//static int nativeBL=30;
	//nativeBL=(nativeBL+5)%256;
	return backlight;
  }

 void LD(AVFrame *pic,AVCodecContext *c,unsigned char bl)
{
	int i=0,j=0;
	//unsigned char y,u1,v1;
	unsigned char *temp_ptr=pic->data[0];
	unsigned char *temp_ptr_u=pic->data[1];
    unsigned char *temp_ptr_v=pic->data[2];	
	unsigned char *temp_ptr_y=Y_data;
	unsigned char *temp_ptr_u1=U_data;
	unsigned char *temp_ptr_v1=V_data;
	
	
	//unsigned char mu;
	unsigned long y_sum=0;
	//float alpha,beta,temp_beta,pow_beta,gamma;
	//float temp_1,temp_2,temp_3;
	
	unsigned char Y_com_tab[256];
	float Y_amp_tab[256];
	

	for (i=0;i<c->height;i++)
	{
		for(j=0;j<c->width;j++)
		{
			y_sum+=temp_ptr[j];
		}
		temp_ptr+=pic->linesize[0];
	}
	mu=y_sum/c->height/c->width;
	//__android_log_print(ANDROID_LOG_INFO,"SDL","Y average: %u",mu);
		
	//alpha=alpha_gamma_table[backlight];
	//gamma=alpha_gamma_table[mu];

	/* if(backlight>=128)
	{
		//b=(float)bl/255*0.5*(2-log(255/a));
		//temp_beta=65025.0/(bl*mu);
		temp_1=(float)255/backlight;
		temp_2=(float)255/mu;
		//b_1=1/b;
		beta=pow(temp_1,gamma)*pow(temp_2,alpha);
	}else
	{
		//temp_beta=(float)65025/((bl+128)*mu);
		temp_1=(float)255/(backlight+128);
		temp_2=(float)255/mu;
		temp_3=2.0;//(float)255/127;
		//b=pow(temp_b,a)+1*pow((float)(255-mu)/255,0.5);
		//beta=pow(temp_beta,alpha)+pow(65025.0/(127*mu),alpha)-pow(255.0/mu,alpha);
		beta=pow(temp_1,gamma)*pow(temp_2,alpha)+pow(temp_3,gamma)*pow(temp_2,alpha)-pow(temp_2,alpha);
	} */
	//pow_beta=pow(255,beta);
	//float pi=3.14159;
	/*此处需添加新的代码: 先实现两档, 让backlight 动态可变, 如果mu<90, 则backlight=90, 否则, backlight=110*/
	backlight=(mu<100)?((backlightOri<20)?0:backlightOri-20):backlightOri;
	//int   intercept_model=214; //越小补偿曲线越陡峭补偿倍数越大.
	int intercept_model=216-5120/mu;
	if(mu>=32)
		intercept_model=216-5120/mu;
	else
		intercept_model=216-5120/32-31/mu;
	int   line_end=ceil(backlight/255*(255-intercept_model)+intercept_model);
    float distance_mu=ceil(mu/line_end*(255-line_end)/1.0); //调分母的数字可以在亮度补偿的客观PSNR和主观质量之间作折中 ceil取上整
    float distance_bl=ceil((255-backlight)/8.0); //调分母的数字可以在亮度补偿的客观PSNR和主观质量之间作折中
	int   line_goback=distance_mu+distance_bl; //line_goback越大第二折线开始的越早越接近黑叶子
	int   line_start=line_end-line_goback;
	float line_slope_first=(255.0/(backlight/255.0*(255.0-intercept_model)+intercept_model));
	//__android_log_print(ANDROID_LOG_INFO,"line_slope_first", "%f",line_slope_first);
	for(i=1;i<=line_start;i++)
	{
	    //Y_com_tab[i]=ceil(i*255/(backlight/255*(255-intercept)+intercept));
		Y_com_tab[i]=ceil(i*line_slope_first);//4次乘法2次加法降为一次乘法.
		//Y_amp_tab[i]=(float)(1.0*Y_com_tab[i]/i);
	}
	float line_slope_second=(255.0-Y_com_tab[line_start])/(255.0-line_start);
	float line_intercept_second=Y_com_tab[line_start]-line_slope_second*line_start;
	//__android_log_print(ANDROID_LOG_INFO,"line_intercept_second", "%f",line_intercept_second);
	for(i=line_start+1;i<=255;i++)
	{
		//Y_com_tab[i]=(float)ceil(Y_com_tab[line_end-line_goback]+(255-Y_com_tab[line_end-line_goback])*sin((i-line_end+line_goback)/(510-2*line_end+2*line_goback)*pi)); 
		//Y_com_tab[i]=ceil(Y_com_tab[line_start]+(255-Y_com_tab[line_start])*(i-line_start)/(255-line_start)); 
		Y_com_tab[i]=ceil(line_intercept_second+line_slope_second*i); //2次乘法4次加法降为一次乘法一次加法.
		//Y_amp_tab[i]=(float)(1.0*Y_com_tab[i]/i);
	}
	/* for(i=1;i<255;i++)
	{
		Y_com_tab[i]=255*(1-pow(255-i,beta)/pow_beta);	
        Y_amp_tab[i]=(float)Y_com_tab[i]/i; //解决精度问题, 原因待查！
		//Y_amp_tab[i]=(float)Y_com_tab[i]/i;
        //Y_amp_tab_float[i]=(float)Y_amp_tab_int[i]/100;		
        //__android_log_print(ANDROID_LOG_INFO,"f1","comp parameter: %u %d",Y_com_tab[i],i);
        //__android_log_print(ANDROID_LOG_INFO,"f2","amp parameter:  %f %d",Y_amp_tab[i],i);		
	}
	Y_com_tab[0]=0;	
	Y_com_tab[255]=255;
	Y_amp_tab[0]=1.0;
	Y_amp_tab[255]=1.0; */
	Y_com_tab[0]=0;	/*必须打开*/
	//Y_amp_tab[0]=1;
	//Y_com_tab[255]=255;
	//Y_amp_tab[255]=1;
	
	//__android_log_print(ANDROID_LOG_INFO,"SDL","begin adjust y");
 	temp_ptr=pic->data[0];
	for (i=0;i<c->height/2;i++)
	{
		for (j=0;j<c->width/2;j++)/*even volume*/
		{
			/*even row*/
			temp_ptr_y[2*j]=Y_com_tab[temp_ptr[2*j]];
			/*odd row*/
			temp_ptr_y[2*j+1]=Y_com_tab[temp_ptr[2*j+1]];
			//int k=pic->linesize[1]*i+j;
			//temp_ptr_u1[k]=(temp_ptr_u[k]-128)*Y_amp_tab[temp_ptr[2*j]]+128;
			//temp_ptr_v1[k]=(temp_ptr_v[k]-128)*Y_amp_tab[temp_ptr[2*j]]+128;
		}
		temp_ptr+=pic->linesize[0];
		temp_ptr_y+=pic->linesize[0];
		for (j=0;j<c->width/2;j++)/*odd volume*/
		{
			temp_ptr_y[2*j]=Y_com_tab[temp_ptr[2*j]];
			temp_ptr_y[2*j+1]=Y_com_tab[temp_ptr[2*j+1]];
		}
		temp_ptr+=pic->linesize[0];
		temp_ptr_y+=pic->linesize[0];
	}
	
	//__android_log_print(ANDROID_LOG_INFO,"SDL","break point2");
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



int
main(int argc, char **argv) {
  AVFormatContext *pFormatCtx;
  int             i,videoStream;
  AVCodecContext  *pCodecCtx;
  AVCodec         *pCodec;
  AVFrame         *pFrame; 
  AVPacket        packet;
  int             frameFinished;
  float           aspect_ratio;
  struct SwsContext      *ctx;
  
  int i_frame=0;
  
  int index=0;
  
  
  SDL_Rect        rect;
  SDL_Event       event;

  SDL_Texture	*streamTex;
  SDL_Window	*pWindow;
  SDL_Renderer	*pRenderer;
  
  //test set bl in c
  //SDL_Thread *blAdjThread=NULL;
  
  unsigned char *pic_y,*pic_u,*pic_v;//用来存储未作处理的y,u,v分量
  
	/* for (i=0;i<50;i++){
		alpha_gamma_table[i]=0.25;
		alpha_gamma_table[i+50]=0.244;
		alpha_gamma_table[i+100]=0.235;
		alpha_gamma_table[i+150]=0.216;
	}
	for (i=0;i<25;i++){
		alpha_gamma_table[i+200]=0.202;
	}
	for (i=0;i<15;i++){
		alpha_gamma_table[i+225]=0.185;
	}
	for (i=0;i<5;i++){
		alpha_gamma_table[i+240]=0.175;
	}
	for (i=0;i<11;i++){
		alpha_gamma_table[i+245]=0.175*(255-i-245)/10;
	} */

  // Register all formats and codecs
  av_register_all();
  
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
	fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
	exit(1);
  }

  // Open video file
  if(av_open_input_file(&pFormatCtx, file, NULL, 0, NULL)!=0)
	return -1; // Couldn't open file
  
  // Retrieve stream information
  if(av_find_stream_info(pFormatCtx)<0)
	return -1; // Couldn't find stream information
  
  // Dump information about file onto standard error
  //dump_format(pFormatCtx, 0, argv[1], 0);
  
  // Find the first video stream
  videoStream=-1;
  for(i=0; i<pFormatCtx->nb_streams; i++)
	if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO) {
	  videoStream=i;
	  break;
	}
  if(videoStream==-1)
	return -1; // Didn't find a video stream
  
  // Get a pointer to the codec context for the video stream
  pCodecCtx=pFormatCtx->streams[videoStream]->codec;
  
  // Find the decoder for the video stream
  pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
  if(pCodec==NULL) {
	//fprintf(stderr, "Unsupported codec!\n");
	__android_log_print(ANDROID_LOG_ERROR,"c","Unsupported codec!");
	return -1; // Codec not found
  }
  
  // Open codec
  if(avcodec_open(pCodecCtx, pCodec)<0)
	return -1; // Could not open codec
  
  // Allocate video frame
  pFrame=avcodec_alloc_frame();
 
  //blAdjThread=SDL_CreateThread(blTestThread,"blTestThread",(void*)NULL);
  //if(blAdjThread==NULL)
//	__android_log_print(ANDROID_LOG_ERROR,"sdl in c","create bl adj thread failed");
  //else
//	__android_log_print(ANDROID_LOG_INFO,"sdl in c"," bl adj thread created");
  // Read frames and use SDL to display the video data(YUV)
  while(1)
  {
	  i_frame=0;
	  int dur_i;
	  clock_t start, finish;
	  double duration;
	  while(av_read_frame(pFormatCtx, &packet)>=0) {
	  start=clock();
		// Is this a packet from the video stream?
		if(packet.stream_index==videoStream) {
		  // Decode video frame
		  avcodec_decode_video(pCodecCtx, pFrame, &frameFinished, 
				   packet.data, packet.size);
		  
		  // Did we get a video frame?
		  if(frameFinished) {
			  if (whKnown_flag==0)
			  {
				  iHeight=pCodecCtx->height;
				  iWidth=pCodecCtx->width;
				  Frame_Y_Size=iWidth*iHeight;
				  whKnown_flag=1;
				  pWindow=SDL_CreateWindow(argv[1],SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
					  iWidth,iHeight,SDL_WINDOW_SHOWN);

				  if (pWindow==NULL)
				  {
					  //fprintf(stderr,"could not create window %\n",SDL_GetError());
					  exit(1);
				  }
				  pRenderer=SDL_CreateRenderer(pWindow,-1,SDL_RENDERER_ACCELERATED);
				  if (pRenderer==NULL)
				  {
					  //fprintf(stderr,"could not create renderer:%s\n",SDL_GetError());
					  exit(1);
				  }
				  streamTex=SDL_CreateTexture(pRenderer,SDL_PIXELFORMAT_YV12,SDL_TEXTUREACCESS_STREAMING,iWidth,iHeight);
				  
				  Y_data=malloc(iHeight*pFrame->linesize[0]);
				  U_data=malloc((iHeight/2)*pFrame->linesize[1]);
				  V_data=malloc((iHeight/2)*pFrame->linesize[2]);
			  }
			  
			 
				
				if(energy_flag==2)
				{
					//bl=BL(picture,c);
					//bl=1;//在此 调节背光级别 选择范围是1-255；不能选0
					
					//做像素点补偿
					LD(pFrame,pCodecCtx,backlight);
					//__android_log_print(ANDROID_LOG_INFO,"SDL","the backlight detected in c is %u",backlight);
					
					pic_y=pFrame->data[0];
					pFrame->data[0]=Y_data;
					//pic_u=pFrame->data[1];
					//pFrame->data[1]=U_data;
					//pic_v=pFrame->data[2];
					//pFrame->data[2]=V_data;
					
					updateTexture(streamTex,pFrame);
					
					pFrame->data[0]=pic_y;
					//pFrame->data[1]=pic_u;
					//pFrame->data[2]=pic_v;
				}else
				{
					//bl=255; 最佳亮度
					//bl = other
					updateTexture(streamTex,pFrame);
				}
				
				//updateTexture(streamTex,pFrame);
				SDL_RenderClear(pRenderer);

				SDL_RenderCopy(pRenderer, streamTex, NULL, NULL);

				SDL_RenderPresent(pRenderer);
                //clock
				i_frame++;
				finish=clock();
				duration = (double)(finish - start) / CLOCKS_PER_SEC; //播放帧率控制
				//__android_log_print(ANDROID_LOG_INFO,"seconds per frame","%f",duration);
				dur_i=(int)(1000*duration);
				//__android_log_print(ANDROID_LOG_INFO,"ms per Frame","%d",dur_i);
				if(dur_i<40){
					SDL_Delay(40-dur_i);
				}
				//clock_t finish_ob=clock();
				//double duration_ob = (double)(finish_ob - start) / CLOCKS_PER_SEC;
				//int dur_i_ob=(int)(1000*duration_ob);
				//__android_log_print(ANDROID_LOG_INFO,"ms per Frame with delay","%d",dur_i_ob); //播放帧率控制
		  }
		}
		av_free_packet(&packet);
		
		while(state==PAUSE){
			SDL_Delay(200);
		}
		if(state==STOP)
		{
			break;				
		}
		
		// Free the packet that was allocated by av_read_frame
		//av_free_packet(&packet);
		/*
		SDL_PollEvent(&event);
		switch(event.type) {
		case SDL_QUIT:
			SDL_DestroyTexture(streamTex);
			SDL_DestroyRenderer(pRenderer);
			//SDL_Delay(5000);
			SDL_DestroyWindow(pWindow);
		  SDL_Quit();
		  exit(0);
		  break;
		  default:
		  break;
		}
		*/
	  }
	av_close_input_file(pFormatCtx);
	// Open video file
	//if(av_open_input_file(&pFormatCtx, file, NULL, 0, NULL)!=0)
		//return -1; // Couldn't open file
	//av_open_input_file(&pFormatCtx, file, NULL, 0, NULL);	
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
	fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
	exit(1);
    }

  // Open video file
  if(av_open_input_file(&pFormatCtx, file, NULL, 0, NULL)!=0)
	return -1; // Couldn't open file
  
  // Retrieve stream information
  if(av_find_stream_info(pFormatCtx)<0)
	return -1; // Couldn't find stream information
  
  // Dump information about file onto standard error
  //dump_format(pFormatCtx, 0, argv[1], 0);
  
  // Find the first video stream
  videoStream=-1;
  for(i=0; i<pFormatCtx->nb_streams; i++)
	if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO) {
	  videoStream=i;
	  break;
	}
  if(videoStream==-1)
	return -1; // Didn't find a video stream
  
  // Get a pointer to the codec context for the video stream
  pCodecCtx=pFormatCtx->streams[videoStream]->codec;
  
  // Find the decoder for the video stream
  pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
  if(pCodec==NULL) {
	//fprintf(stderr, "Unsupported codec!\n");
	__android_log_print(ANDROID_LOG_ERROR,"c","Unsupported codec!");
	return -1; // Codec not found
  }
  
  // Open codec
  if(avcodec_open(pCodecCtx, pCodec)<0)
	return -1; // Could not open codec
  
  // Allocate video frame
  pFrame=avcodec_alloc_frame();
   
	if(state==STOP)
	{
		SDL_DestroyTexture(streamTex);
		SDL_DestroyRenderer(pRenderer);
		SDL_DestroyWindow(pWindow);
		SDL_Quit();
		exit(0);
		break;
	} 
  }
  
  // Free the YUV frame
	av_free(pFrame);
	free(Y_data);
	free(U_data);
	free(V_data);
	
	// Close the codec
	avcodec_close(pCodecCtx);

	// Close the video file
	

	return 0;
}
