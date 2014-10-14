#include <pthread.h>
#include <stdlib.h>

#define READING 0
#define WRITTEN 1
#define READDONE 2
#define WRITING 3

typedef struct BBuffer{
	unsigned char *bufs[2];
	int rwStat[2];
	int bufSize[2];
	int bufToRead;
	int bufToWrite;
	pthread_mutex_t mutex;
	pthread_cond_t cannotWrite;
	pthread_cond_t cannotRead;
} BBuffer;

void initBBuffer(BBuffer *bBuffer,size_t length);

void releaseBBuffer(BBuffer *bBuffer);

int readBBuffer(BBuffer *bBuffer,unsigned char buf[]);

void writeBBuffer(BBuffer *bBuffer,unsigned char buf[],int size);
