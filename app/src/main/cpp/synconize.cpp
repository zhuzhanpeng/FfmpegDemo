#include "synconize.h"
#include <queue>
#include "FFmpegVideo.h"
#include "FFmpegMusic.h"

#include <libavformat/avformat.h>
extern "C" {
#include <pthread.h>
#include <unistd.h>
using namespace std;
//这是什么意思
#ifndef _Nonnull
#define _Nonnull
#endif

std::queue<AVPacket*> audio_queue, video_queue;

AVFormatContext *ifmt_ctx;
AVCodecContext *codec_ctx;
AVPacket *readPkt;
FFmpegMusic audio;
FFmpegVideo video;


void *fill_stack(const char* path) {
    av_register_all();
    ifmt_ctx=avformat_alloc_context();
    if(avformat_open_input(&ifmt_ctx,path,NULL,NULL)<0){
        return (void *) -1;
    }

    if(avformat_find_stream_info(ifmt_ctx,NULL)<0){
        return (void *) -1;
    }

    int video_id=-1;
    int audio_id=-1;

    for (int i = 0; i < ifmt_ctx->nb_streams; ++i) {
        if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
            video_id=i;
        }
        if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
            audio_id=i;
        }
    }

    if(video_id==-1){
        return (void *) -1;
    }

    if(audio_id==-1){
        return (void *) -1;
    }

    codec_ctx=ifmt_ctx->streams[video_id]->codec;

    AVCodec *avCodec = avcodec_find_decoder(codec_ctx->codec_id);
    if(avcodec_open2(codec_ctx,avCodec,NULL)<0){
        return (void *) -1;
    }

    while(av_read_frame(ifmt_ctx,readPkt)>=0){

        if(readPkt->stream_index==video_id){
            video_queue.push(readPkt);
        }
        if(readPkt->stream_index==audio_id){
            audio_queue.push(readPkt);
        }
    }

}
void *audioSynVideo(const char *path) {
    pthread_t pid;
    pthread_create(&pid,NULL,fill_stack,path);
    int* ret=0;
    pthread_join(pid, (void **) &ret);
}

}



