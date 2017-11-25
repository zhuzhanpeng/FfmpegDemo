//
// Created by Administrator on 2017/11/22.
//

#ifndef FFMPEGDEMO_FFMPEGVIDEO_H
#define FFMPEGDEMO_FFMPEGVIDEO_H


extern "C"{
#include <libavformat/avformat.h>

};
#endif //FFMPEGDEMO_FFMPEGVIDEO_H

#include <pthread.h>
#include <queue>


class FFmpegVideo {
//方法
public:
    FFmpegVideo();
    ~FFmpegVideo();
    void put(AVPacket* packet);
    void get(AVPacket* packet);
    void play();
    void release();
    void setPlayFrame(void (*play_frame)(AVFrame* frame));
    //字段
public:
    double clock;
    pthread_t v_tid;
    std::queue<AVPacket*> queue;
    pthread_mutex_t v_mutex;
    int is_playing;
    AVRational time_base;
    AVCodecContext* codec_ctx;
};