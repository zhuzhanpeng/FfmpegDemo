#include <jni.h>
#include <string>
#include <android/log.h>
#include "log.h"
#include "android/native_window_jni.h"


#ifndef _Included_com_dongnao_ffmpegdemo_MainActivity
#define _Included_com_dongnao_ffmpegdemo_MainActivity
#ifdef __cplusplus
extern "C" {
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//像素处理
#include "libswscale/swscale.h"
#endif

/*
 * Class:     com_dongnao_ffmpegdemo_MainActivity
 * Method:    playVideo
 * Signature: (Ljava/lang/String;Landroid/view/Surface;)V
 */
JNIEXPORT void JNICALL Java_com_dongnao_ffmpegdemo_MainActivity_playNativeVideo
        (JNIEnv *env, jobject instance,jstring path_, jobject surface){
    const char *path = env->GetStringUTFChars(path_, 0);
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);

    av_register_all();
    AVFormatContext *formatCtx = avformat_alloc_context();
    if (avformat_open_input(&formatCtx, path, NULL, NULL) < 0) {
        LOGE("avformat_open_input失败");
        return;
    }
    if (avformat_find_stream_info(formatCtx, NULL) < 0) {
        LOGE("avformat_find_stream_info失败");
        return;
    }
    int video_id = -1;
    int i;
    for (i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_id = i;
        }
    }
    if (video_id == -1) {
        return;
    }
    ANativeWindow_Buffer aNativeWindow_buffer;
    AVCodecContext *codecCtx = formatCtx->streams[video_id]->codec;
    AVCodec *videoDecoder = avcodec_find_decoder(codecCtx->codec_id);

    AVFrame *frame = av_frame_alloc();
    AVFrame *yuvFrame = av_frame_alloc();
    uint8_t *buffer = (uint8_t *) av_mallocz(
            avpicture_get_size(AV_PIX_FMT_RGBA, codecCtx->width, codecCtx->height));
    avpicture_fill((AVPicture *) yuvFrame, (const uint8_t *) buffer, AV_PIX_FMT_RGBA,
                   codecCtx->width, codecCtx->height);
    AVPacket *packet = (AVPacket *) av_mallocz(sizeof(AVPacket));

    SwsContext *swsCtx = sws_getContext(codecCtx->width, codecCtx->height,
                                        codecCtx->pix_fmt,
                                        codecCtx->width, codecCtx->height, AV_PIX_FMT_RGBA,
                                        SWS_BILINEAR, NULL, NULL, NULL);
    int got_frame = -1;
    while (av_read_frame(formatCtx, packet) >= 0) {
        avcodec_decode_video2(codecCtx, frame, &got_frame, packet);
        if (got_frame != 0) {
            ANativeWindow_setBuffersGeometry(nativeWindow,
                                             codecCtx->width,
                                             codecCtx->height, WINDOW_FORMAT_RGBA_8888);
            ANativeWindow_lock(nativeWindow, &aNativeWindow_buffer, NULL);
            sws_scale(swsCtx, (const uint8_t *const *) frame->data, frame->linesize, 0,
                      frame->height, yuvFrame->data, yuvFrame->linesize);
            uint8_t *dst = (uint8_t *) aNativeWindow_buffer.bits;
            int dstStride = aNativeWindow_buffer.stride * 4;

            uint8_t *src = yuvFrame->data[0];
            int srcStride = yuvFrame->linesize[0];
            int i = 0;
            for (; i < codecCtx->height; i++) {
                memcpy(dst + i * dstStride, src + i*srcStride, srcStride);
            }
            ANativeWindow_unlockAndPost(nativeWindow);

        }
    }

    ANativeWindow_release(nativeWindow);
    av_free_packet(packet);
    av_frame_free(&yuvFrame);
    av_frame_free(&frame);
    avformat_free_context(formatCtx);
    env->ReleaseStringUTFChars(path_, path);
}

/*
 * Class:     com_dongnao_ffmpegdemo_MainActivity
 * Method:    playAudio
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_dongnao_ffmpegdemo_MainActivity_playNativeAudio
        (JNIEnv *env, jobject instance, jstring path_){}

/*
 * Class:     com_dongnao_ffmpegdemo_MainActivity
 * Method:    syncronize
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_dongnao_ffmpegdemo_MainActivity_nativeSyncronize
        (JNIEnv *env, jobject instance, jstring path_){}

#ifdef __cplusplus
}
#endif
#endif

