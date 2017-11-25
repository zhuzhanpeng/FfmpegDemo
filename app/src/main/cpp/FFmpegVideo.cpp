#include "FFmpegVideo.h"
#include "log.h"
#include "android/native_window_jni.h"
extern "C"{
#include <libswscale/swscale.h>
}
ANativeWindow *nativeWindow;
FFmpegVideo::FFmpegVideo(){

}
FFmpegVideo::~FFmpegVideo(){

}
static void (*video_call)(AVFrame *frame);
void* play_callback(void* argc){
    FFmpegVideo* video = (FFmpegVideo*)argc;

    AVFrame *frame = av_frame_alloc();
    AVFrame *yuvFrame = av_frame_alloc();
    uint8_t *buffer = (uint8_t *) av_mallocz(
            avpicture_get_size(AV_PIX_FMT_RGBA, video->codec_ctx->width, video->codec_ctx->height));
    avpicture_fill((AVPicture *) yuvFrame, (const uint8_t *) buffer, AV_PIX_FMT_RGBA,
                   video->codec_ctx->width, video->codec_ctx->height);
    AVPacket *packet = (AVPacket *) av_mallocz(sizeof(AVPacket));

    SwsContext *swsCtx = sws_getContext(video->codec_ctx->width, video->codec_ctx->height,
                                        video->codec_ctx->pix_fmt,
                                        video->codec_ctx->width, video->codec_ctx->height, AV_PIX_FMT_RGBA,
                                        SWS_BILINEAR, NULL, NULL, NULL);
    int got_frame = -1;
    while (video->is_playing) {
        video->get(packet);
        avcodec_decode_video2(video->codec_ctx, frame, &got_frame, packet);
        if (got_frame != 0) {
            sws_scale(swsCtx, (const uint8_t *const *) frame->data, frame->linesize, 0,
                      frame->height, yuvFrame->data, yuvFrame->linesize);
            video_call(yuvFrame);
        }
    }


}
void FFmpegVideo::put(AVPacket* packet){
    AVPacket* copy_pkt = (AVPacket *) av_malloc(sizeof(AVPacket));
    if(av_copy_packet(copy_pkt,packet)){
        LOGE("克隆失败");
        return;
    }
    queue.push(copy_pkt);
}
void FFmpegVideo::play(){
    is_playing=1;
    //开线程播放
    pthread_create(&v_tid,0,play_callback,this);
}
void FFmpegVideo::setPlayFrame(void (*play_frame)(AVFrame* frame)){
    video_call=play_frame;
}
void FFmpegVideo::get(AVPacket* packet){
    while (is_playing){
        if(!queue.empty()){
            av_packet_ref(packet,queue.front());
        }else{

        }
    }
}

void FFmpegVideo::release(){
}