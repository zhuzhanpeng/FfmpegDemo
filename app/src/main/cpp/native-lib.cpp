#include <jni.h>
#include <string>
#include "synconize.h"
#include "log.h"
#include <pthread.h>
#include "android/native_window_jni.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "SplitVideo.h"

#ifndef _Included_com_dongnao_ffmpegdemo_MainActivity
#define _Included_com_dongnao_ffmpegdemo_MainActivity
#endif
SLEnvironmentalReverbSettings settings;
SLObjectItf engineObject;
SLEngineItf engineInterface;
SLObjectItf outputMix;
// buffer queue player interfaces
SLObjectItf bqPlayerObject = NULL;
//播放接口
SLPlayItf bqPlayerPlay;
SLEnvironmentalReverbItf outputMixEnvironmentalReverb;
//缓冲器队列接口
SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;

SLVolumeItf bqPlayerVolume;
void *buffer;
size_t bufferSize = 0;

extern "C" {

//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//处理音频
#include "libswresample/swresample.h"
//像素处理
#include "libswscale/swscale.h"
#include <libavutil/timestamp.h>
}

// 当喇叭播放完声音时回调此方法
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context){
    bufferSize=0;
//    取到音频数据了
//    getPCM(&buffer, &bufferSize);
    if (NULL != buffer && 0 != bufferSize) {
//        播放的关键地方
        SLresult  lresult=(*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, buffer, bufferSize);
        LOGE("正在播放%d ",lresult);
    }
}
/*
 * Class:     com_dongnao_ffmpegdemo_MainActivity
 * Method:    playVideo
 * Signature: (Ljava/lang/String;Landroid/view/Surface;)V
 */
extern "C"
JNIEXPORT void JNICALL Java_com_dongnao_ffmpegdemo_MainActivity_playNativeVideo
        (JNIEnv *env, jobject instance, jstring path_, jobject surface) {
    const char *input = env->GetStringUTFChars(path_, 0);
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);

    av_register_all();
//    char *input = "/sdcard/input.mp4";
    AVFormatContext *formatCtx = avformat_alloc_context();
    if (avformat_open_input(&formatCtx, input, NULL, NULL) < 0) {
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
    if (avcodec_open2(codecCtx, videoDecoder, NULL) < 0) {
        return;
    }
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
                memcpy(dst + i * dstStride, src + i * srcStride, srcStride);
            }
            ANativeWindow_unlockAndPost(nativeWindow);

        }
    }

    ANativeWindow_release(nativeWindow);
    sws_freeContext(swsCtx);
    av_free_packet(packet);
    av_frame_free(&yuvFrame);
    av_frame_free(&frame);
    avformat_free_context(formatCtx);
    env->ReleaseStringUTFChars(path_, input);
}

/*
 * Class:     com_dongnao_ffmpegdemo_MainActivity
 * Method:    playAudio
 * Signature: (Ljava/lang/String;)V
 */
extern "C"
JNIEXPORT void JNICALL Java_com_dongnao_ffmpegdemo_MainActivity_playNativeAudio
        (JNIEnv *env, jobject instance, jstring path_) {
    const char *path = env->GetStringUTFChars(path_, NULL);

    SLresult sLresult;
    slCreateEngine(&engineObject, NULL, NULL, NULL, NULL, NULL);

    (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    //获取SLEngine借口对象
    (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
    (*engineInterface)->CreateOutputMix(engineInterface, &outputMix, 0, 0, 0);

    (*outputMix)->Realize(outputMix, SL_BOOLEAN_FALSE);
    (*outputMix)->GetInterface(outputMix, SL_IID_ENVIRONMENTALREVERB,
                               &outputMixEnvironmentalReverb);

    if (SL_RESULT_SUCCESS == sLresult) {
        (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &settings);
    }

    //采样率
    int rate;
    //声道数
    int channels;
//    createFFmpeg(&rate, &channels);
    LOGE(" 比特率%d  ,channels %d", rate, channels);
    //  配置信息设置
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};
    SLDataFormat_PCM pcm={SL_DATAFORMAT_PCM,2,SL_SAMPLINGRATE_44_1,SL_PCMSAMPLEFORMAT_FIXED_16
            ,SL_PCMSAMPLEFORMAT_FIXED_16,SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,SL_BYTEORDER_LITTLEENDIAN};
    //   新建一个数据源 将上述配置信息放到这个数据源中
    SLDataSource slDataSource = {&android_queue, &pcm};
    //    设置混音器
    SLDataLocator_OutputMix slDataLocator_outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMix};
    SLDataSink dataSink={&slDataLocator_outputMix,NULL};

    //创建Recorder需要 RECORD_AUDIO 权限
//    SLInterfaceID slInterfaceID[2]={SL_IID_ANDROIDSIMPLEBUFFERQUEUE,SL_IID_ANDROIDCONFIGURATION};
//    SLboolean reqs[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    // create audio player
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
            /*SL_IID_MUTESOLO,*/ SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
            /*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE};

    int reslut=SL_RESULT_SUCCESS==sLresult;

    sLresult=(*engineInterface)->CreateAudioPlayer(engineInterface,&bqPlayerObject,&slDataSource,
    &dataSink,3,ids,req);
    (*bqPlayerObject)->Realize(bqPlayerObject,SL_BOOLEAN_FALSE);
    (*bqPlayerObject)->GetInterface(bqPlayerObject,SL_IID_PLAY,&bqPlayerPlay);
    //注册回调缓冲区，获取缓冲队列借口
    (*bqPlayerObject)->GetInterface(bqPlayerObject,SL_IID_BUFFERQUEUE,
    &bqPlayerBufferQueue);

    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue,bqPlayerCallback,NULL);
    (*bqPlayerObject)->GetInterface(bqPlayerObject,SL_IID_VOLUME,&bqPlayerVolume);

    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay,SL_PLAYSTATE_PLAYING);
    bqPlayerCallback(bqPlayerBufferQueue,NULL);

    env->ReleaseStringUTFChars(path_, path);
}

/*
 * Class:     com_dongnao_ffmpegdemo_MainActivity
 * Method:    syncronize
 * Signature: (Ljava/lang/String;)V
 */
extern "C"
JNIEXPORT void JNICALL Java_com_dongnao_ffmpegdemo_MainActivity_nativeSyncronize
        (JNIEnv *env, jobject instance, jstring path_,jobject surface) {
    const char* path=env->GetStringUTFChars(path_,NULL);
    ANativeWindow* nativeWindow= ANativeWindow_fromSurface(env, surface);
    audioSynVideo(path,nativeWindow);
    env->ReleaseStringUTFChars(path_,path);
}
static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;
    printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           tag,
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}
extern "C"
JNIEXPORT void JNICALL Java_com_dongnao_ffmpegdemo_MainActivity_nativeTranscoding
        (JNIEnv *env, jobject instance, jstring path_) {
    const char* path=env->GetStringUTFChars(path_,NULL);


    const char *in_filename="/sdcard/input.mp4";
    /*const char *in_filename=
            "http://img.paas.onairm.cn/8abcbfaad1e48708b94466e024e24629base?avvod/m3u8/s/640*960/vb/400k/autosave/1";*/
//    const char* in_filename="http://joymedia.oss-cn-hangzhou.aliyuncs.com/joyMedia/live_id_41.m3u8";
    const char *out_filename="/sdcard/m3u8.flv";
    AVOutputFormat *ofmt = NULL;
    //Input AVFormatContext and Output AVFormatContext
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVPacket pkt;

    int ret, i;
    int frame_index=0;

    av_register_all();
    avformat_network_init();
    //Input
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        LOGE("Could not open input file.");
        goto end;
    }
    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        LOGE( "Failed to retrieve input stream information");
        goto end;
    }
    av_dump_format(ifmt_ctx, 0, in_filename, 0);
    //Output
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx) {
        LOGE( "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    ofmt = ofmt_ctx->oformat;
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        //Create output AVStream according to input AVStream
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
        if (!out_stream) {
            LOGE( "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        //Copy the settings of AVCodecContext
        if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
            LOGE( "Failed to copy context from input to output stream codec context\n");
            goto end;
        }
        out_stream->codec->codec_tag = 0;
        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    //Output information------------------
    av_dump_format(ofmt_ctx, 0, out_filename, 1);
    //Open output file
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE( "Could not open output file '%s'", out_filename);
            goto end;
        }
    }
    //Write file header
    if (avformat_write_header(ofmt_ctx, NULL) < 0) {
        LOGE( "Error occurred when opening output file\n");
        goto end;
    }

    while (1) {
        AVStream *in_stream, *out_stream;
        //Get an AVPacket
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0)
            break;
        in_stream  = ifmt_ctx->streams[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];

        //Convert PTS/DTS
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        //Write
        if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
            LOGE( "Error muxing packet\n");
            break;
        }
        LOGE("Write %8d frames to output file\n",frame_index);
        av_free_packet(&pkt);
        frame_index++;
    }
    //Write file trailer
    av_write_trailer(ofmt_ctx);
    end:
    avformat_close_input(&ifmt_ctx);
    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);

    env->ReleaseStringUTFChars(path_,path);
}
extern "C"
JNIEXPORT void JNICALL Java_com_dongnao_ffmpegdemo_MainActivity_nativeSplitVideo
        (JNIEnv *env, jobject instance, jstring path_) {
    executeSplitOneClip(60,100);
}


