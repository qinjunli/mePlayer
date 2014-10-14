#include <sys/time.h>
#include "bBuffer.h"

void initBBuffer(BBuffer *bBuffer,size_t length){
	bBuffer->bufs[0]=(unsigned char *)malloc(length);
	bBuffer->bufs[1]=(unsigned char *)malloc(length);
	bBuffer->bufToRead=0;
	bBuffer->bufToWrite=0;
	bBuffer->rwStat[0]=READDONE;
	bBuffer->rwStat[1]=READDONE;
	pthread_mutex_init(&bBuffer->mutex,NULL);
	pthread_cond_init(&bBuffer->cannotWrite,NULL);
	pthread_cond_init(&bBuffer->cannotRead,NULL);
}

void releaseBBuffer(BBuffer *bBuffer){
	free(bBuffer->bufs[0]);
	free(bBuffer->bufs[1]);
}

int readBBuffer(BBuffer *bBuffer,unsigned char buf[]){
	struct timeval now;
	struct timespec outtime;
	gettimeofday(&now,NULL);
	outtime.tv_sec=now.tv_sec+1;
	outtime.tv_nsec=now.tv_usec*1000;
	
	int i;
	pthread_mutex_lock(&bBuffer->mutex);
	
	if(bBuffer->rwStat[bBuffer->bufToRead]!=WRITTEN){
		pthread_cond_timedwait(&bBuffer->cannotRead,&bBuffer->mutex,&outtime);
	}
	/*
	while(bBuffer->rwStat[bBuffer->bufToRead]!=WRITTEN){
		pthread_cond_wait(&bBuffer->cannotRead,&bBuffer->mutex);
	}
	*/
	bBuffer->rwStat[bBuffer->bufToRead]=READING;
	
	pthread_mutex_unlock(&bBuffer->mutex);

	memcpy(buf,bBuffer->bufs[bBuffer->bufToRead],bBuffer->bufSize[bBuffer->bufToRead]);
	/*
	for(i=0;i<bBuffer->bufSize[bBuffer->bufToRead];i++){
		buf[i]=bBuffer->bufs[bBuffer->bufToRead][i];
	}
	*/
	
	pthread_mutex_lock(&bBuffer->mutex);
	int length=bBuffer->bufSize[bBuffer->bufToRead];
	bBuffer->bufSize[bBuffer->bufToRead]=0;
	bBuffer->rwStat[bBuffer->bufToRead]=READDONE;
	bBuffer->bufToRead=1-bBuffer->bufToRead;
	pthread_cond_signal(&bBuffer->cannotWrite);
	
	pthread_mutex_unlock(&bBuffer->mutex);
	
	return length;
}

void writeBBuffer(BBuffer *bBuffer,unsigned char buf[],int size){
	int i;
	pthread_mutex_lock(&bBuffer->mutex);
	if(bBuffer->rwStat[bBuffer->bufToWrite]!=READDONE){
		pthread_cond_wait(&bBuffer->cannotWrite,&bBuffer->mutex);
	}
	/*
	while(bBuffer->rwStat[bBuffer->bufToWrite]!=READDONE){
		pthread_cond_wait(&bBuffer->cannotWrite,&bBuffer->mutex);
	}
	*/
	bBuffer->rwStat[bBuffer->bufToWrite]=WRITING;
	
	pthread_mutex_unlock(&bBuffer->mutex);
	
	memcpy(bBuffer->bufs[bBuffer->bufToWrite],buf,size);
	/*
	for(i=0;i<size;i++){
		bBuffer->bufs[bBuffer->bufToWrite][i]=buf[i];
	}
	*/
	
	pthread_mutex_lock(&bBuffer->mutex);
	
	bBuffer->bufSize[bBuffer->bufToWrite]=size;
	bBuffer->rwStat[bBuffer->bufToWrite]=WRITTEN;
	bBuffer->bufToWrite=1-bBuffer->bufToWrite;
	pthread_cond_signal(&bBuffer->cannotRead);
	
	pthread_mutex_unlock(&bBuffer->mutex);
}