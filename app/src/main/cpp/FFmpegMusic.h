//
// Created by Administrator on 2017/11/13.
//
#include <unistd.h>

#ifndef FFMPEGDEMO_FFMPEGMUSIC_H
#define FFMPEGDEMO_FFMPEGMUSIC_H

#endif //FFMPEGDEMO_FFMPEGMUSIC_H

#include <pthread.h>
extern "C"{
#include <libavformat/avformat.h>
};
#include <queue>

class FFmpegMusic{
    //方法
public:
    FFmpegMusic();
    ~FFmpegMusic();
    void play();
    void put(AVPacket* packet);
    int createFFmpeg(int* rate,int* channels);
    void getPCM(void **outBuffer,size_t *size);
    void release();
    //字段
public:
    double clock;
    pthread_t a_tid;
    int is_playing;
    std::queue<AVPacket*> queue;
    AVRational time_base;
    AVCodecContext codec_ctx;
};
