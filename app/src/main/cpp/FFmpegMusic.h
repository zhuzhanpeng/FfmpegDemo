//
// Created by Administrator on 2017/11/13.
//
#include <unistd.h>

#ifndef FFMPEGDEMO_FFMPEGMUSIC_H
#define FFMPEGDEMO_FFMPEGMUSIC_H

#endif //FFMPEGDEMO_FFMPEGMUSIC_H

int createFFmpeg(int* rate,int* channels);

void getPCM(void **outBuffer,size_t *size);

void release();