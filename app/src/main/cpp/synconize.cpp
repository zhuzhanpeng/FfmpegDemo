#include <jni.h>
#include "synconize.h"
#include "log.h"
#include "FFmpegVideo.h"
#include "FFmpegMusic.h"
#include "coffeecatch.h"

using namespace std;


ANativeWindow*  nativeWindow;
AVFormatContext *ifmt_ctx;
AVCodecContext *codec_ctx;
AVPacket *readPkt;
FFmpegMusic *audio;
FFmpegVideo *video;

void play_frame(AVFrame *rgb_frame) {
    LOGE("step9");
    if (!nativeWindow){
        LOGE("step8")
        return;
    }
    ;
    ANativeWindow_Buffer aNativeWindow_buffer;
    ANativeWindow_lock(nativeWindow, &aNativeWindow_buffer, NULL);
    uint8_t *dst = (uint8_t *) aNativeWindow_buffer.bits;
    int dstStride = aNativeWindow_buffer.stride * 4;

    uint8_t *src = rgb_frame->data[0];
    int srcStride = rgb_frame->linesize[0];
    int i = 0;
    for (; i < aNativeWindow_buffer.height; i++) {
        memcpy(dst + i * dstStride, src + i * srcStride, srcStride);
    }
    ANativeWindow_unlockAndPost(nativeWindow);

}

void *fill_stack(void *path) {
    av_register_all();
    ifmt_ctx = avformat_alloc_context();
    if (avformat_open_input(&ifmt_ctx, (const char *) path, NULL, NULL) < 0) {
        return (void *) -1;
    }

    if (avformat_find_stream_info(ifmt_ctx, NULL) < 0) {
        return (void *) -1;
    }

    int video_id = -1;
    int audio_id = -1;
    LOGE("step4");
    for (int i = 0; i < ifmt_ctx->nb_streams; ++i) {
        AVCodecContext *pCodeCtx = ifmt_ctx->streams[i]->codec;
        AVCodec *pCodec = avcodec_find_decoder(pCodeCtx->codec_id);

        AVCodecContext *codec = avcodec_alloc_context3(pCodec);
        avcodec_copy_context(codec, pCodeCtx);
        if (avcodec_open2(pCodeCtx, pCodec, NULL) < 0) {
            LOGE("%s", "解码器无法打开");
            continue;
        }
        if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            ANativeWindow_setBuffersGeometry(nativeWindow,
                                             pCodeCtx->width, pCodeCtx->height,
                                             WINDOW_FORMAT_RGBA_8888);
            video_id = i;
            video->time_base = ifmt_ctx->streams[i]->codec->time_base;
            video->codec_ctx = codec;
        }
        if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_id = i;
            audio->time_base = ifmt_ctx->streams[i]->codec->time_base;
            audio->codec_ctx;
        }
    }

    if (video_id == -1) {
        return (void *) -1;
    }

    if (audio_id == -1) {
        return (void *) -1;
    }

    codec_ctx = ifmt_ctx->streams[video_id]->codec;

    AVCodec *avCodec = avcodec_find_decoder(codec_ctx->codec_id);
    if (avcodec_open2(codec_ctx, avCodec, NULL) < 0) {
        return (void *) -1;
    }

    video->play();
//    audio->play();
    readPkt= (AVPacket *) malloc(sizeof(AVPacket));
    while (av_read_frame(ifmt_ctx, readPkt) >= 0) {

        if (readPkt->stream_index == video_id) {
            video->put(readPkt);
        }
        if (readPkt->stream_index == audio_id) {
            audio->put(readPkt);
        }
    }
}
void* ptr_run(void* argc){
    for (int i = 0; i < 100; ++i) {
        LOGE("add%d",i);
    }
}
extern "C"
JNIEXPORT void JNICALL Java_com_dongnao_ffmpegdemo_MainActivity_nativeSyncronize
        (JNIEnv *env, jobject instance, jstring path_,jobject surface) {
    const char* path=env->GetStringUTFChars(path_,NULL);
    nativeWindow= ANativeWindow_fromSurface(env, surface);
    video = new FFmpegVideo();
    video->setPlayFrame(play_frame);
    audio = new FFmpegMusic();
    pthread_t pid;
    pthread_create(&pid, NULL, ptr_run, (void *) path);
    LOGE("zzp");
    env->ReleaseStringUTFChars(path_,path);
}





