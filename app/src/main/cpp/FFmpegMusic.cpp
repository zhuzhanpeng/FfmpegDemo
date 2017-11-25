//
// Created by Administrator on 2017/11/14.
#include "FFmpegMusic.h"


extern "C"{
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//处理音频
#include "libswresample/swresample.h"
//像素处理
#include "libswscale/swscale.h"
}



AVFormatContext *formatCtx;
int audio_id = -1;
AVFrame *frame;
AVPacket *packet;
AVCodecContext *codecCtx;

SwrContext *swrCtx;
uint8_t *out_buffer;

FFmpegMusic::FFmpegMusic(){

}
FFmpegMusic::~FFmpegMusic(){

}
void FFmpegMusic::play(){

}
void FFmpegMusic::put(AVPacket* packet){

}

int FFmpegMusic::createFFmpeg(int *rate, int *channels) {
    char *input = "/sdcard/input.mp4";
    av_register_all();
    formatCtx = avformat_alloc_context();
    if (avformat_open_input(&formatCtx, input, NULL, NULL) < 0) {
        return -1;
    }
    if (avformat_find_stream_info(formatCtx, NULL) < 0) {
        return -1;
    }
    int i;
    for (i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_id = i;
        }
    }
    if (audio_id == -1) {
        return -1;
    }
    codecCtx = formatCtx->streams[audio_id]->codec;
    AVCodec *avDecoder = avcodec_find_decoder(codecCtx->codec_id);

    if (avcodec_open2(codecCtx, avDecoder, NULL) < 0) {
        return -1;
    }

    frame = av_frame_alloc();
    packet = (AVPacket *) av_mallocz(sizeof(AVPacket));
    swrCtx = swr_alloc();
    //    输出采样位数
    //输出的采样率必须与输入相同

    swr_alloc_set_opts(swrCtx, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, codecCtx->sample_rate,
                       codecCtx->channel_layout, codecCtx->sample_fmt, codecCtx->sample_rate, 0,
                       0);
    swr_init(swrCtx);
    *rate = codecCtx->sample_rate;
    *channels = codecCtx->channels;
    out_buffer = (uint8_t *) av_malloc(44100 * 2);
    return 0;
}

void getPCM(void **outBuffer, size_t *size) {
//    输出文件
    int frameCount=0;
    int got_frame;
    int out_channer_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    while (av_read_frame(formatCtx, packet) >= 0) {
        if (packet->stream_index == audio_id) {
            //            解码  mp3   编码格式frame----pcm   frame
            avcodec_decode_audio4(codecCtx, frame, &got_frame, packet);
            if (got_frame) {

                /**
                 * int swr_convert(struct SwrContext *s, uint8_t **out, int out_count,
                                const uint8_t **in , int in_count);
                 */
                swr_convert(swrCtx, &out_buffer, 44100 * 2, (const uint8_t **) frame->data, frame->nb_samples);
                //                通道数
                int out_buffer_size=av_samples_get_buffer_size(NULL, out_channer_nb, frame->nb_samples,AV_SAMPLE_FMT_S16, 1);
                *outBuffer =out_buffer;
                *size =out_buffer_size;
                break;
            }
        }
    }
}

void release() {
    avformat_free_context(formatCtx);
}

