
#include "multi_merge_video.h"
#include "log.h"
/*#define __STDC_CONSTANT_MACROS*/

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
};


void cancatVideo(const char* input[],const int count) {
    AVOutputFormat *ofmt;
    AVFormatContext *ofmt_ctx = NULL;
    AVFormatContext *ifmt_ctx_array[5] = {0};
    for(int i=0;i<count;i++){
        ifmt_ctx_array[i]=avformat_alloc_context();
    }
    AVPacket pkt;
    char out_filename[50] = "/sdcard/merge.mp4";
    int ret, i;
    int frame_index = 0;
/*
    char* input[5]={0};
    input[0]="/sdcard/combine1.mp4";
    input[1]="/sdcard/combine1.mp4";
    input[2]="/sdcard/combine1.mp4";
    input[3]="/sdcard/combine1.mp4";
    input[4]="/sdcard/combine1.mp4";*/

    av_register_all();


    for (int i=0;i< count;i++) {

        /*if (!fopen(input[i],"rb")) {
            LOGE("文件不存在\n");
        } else {
            LOGE("文件是存在的\n");
        }*/
        if ((ret = avformat_open_input(&ifmt_ctx_array[i], input[i], 0, 0)) < 0) {
            LOGE("Could not open input file.%s", input[i]);
//            goto end;
        }
        if ((ret = avformat_find_stream_info(ifmt_ctx_array[i], 0)) < 0) {
            printf("Failed to retrieve input stream information");
//            goto end;
        }
        LOGE("执行到了吗");
    }
//Output
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx) {
        LOGE("Could not create output context\n");
        ret = AVERROR_UNKNOWN;
//        goto end;
    }
    ofmt = ofmt_ctx->oformat;
    for (i = 0; i < ifmt_ctx_array[0]->nb_streams; i++) {
//Create output AVStream according to input AVStream
        AVStream *in_stream = ifmt_ctx_array[0]->streams[i];
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
        if (!out_stream) {
            LOGE("Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
//            goto end;
        }
//Copy the settings of AVCodecContext
        if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
            LOGE("Failed to copy context from input to output stream codec context\n");
//            goto end;
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
            printf("Could not open output file '%s'", out_filename);
//            goto end;
        }
    }
//Write file header
    if (avformat_write_header(ofmt_ctx, NULL) < 0) {
        printf("Error occurred when opening output file\n");
//        goto end;
    }
    AVFormatContext *input_ctx = ifmt_ctx_array[0];
//AVPacket pkt;
    int pts_v = 0, pts_a = 0, dts_v = 0, dts_a = 0;
    float vedioDuraTime = 0.0, audioDuraTime = 0.0;
    int pts_v_temp = 0, pts_a_temp = 0, dts_v_temp = 0, dts_a_temp = 0;
    int vedioIndex = 0;
    while (1) {
        AVStream *in_stream, *out_stream;
//Get an AVPacket
        ret = av_read_frame(input_ctx, &pkt);
        if (ret < 0) {
            if (input_ctx != ifmt_ctx_array[count - 1]) {


//calc the first media dura time
                vedioDuraTime = ((float) input_ctx->streams[0]->time_base.num /
                                 (float) input_ctx->streams[0]->time_base.den) *
                                ((float) pts_v);
                audioDuraTime = ((float) input_ctx->streams[1]->time_base.num /
                                 (float) input_ctx->streams[1]->time_base.den) *
                                ((float) pts_a);
                printf("\nvedioDuraTime=%f\naudioDuraTime=%f\n", vedioDuraTime, audioDuraTime);
//                pts_v_temp += pts_v;
//                pts_v_temp+=1024;
//                dts_v_temp = pts_v_temp;
//                printf("\npts_v_temp=%d\npts_v_temp=%d\n",pts_v_temp,dts_v_temp);
//vedioDuraTime++;
//audioDuraTime++;
//calc the pts and dts end of the first media
                if (audioDuraTime > vedioDuraTime) {
                    dts_v = pts_v =
                            audioDuraTime / ((float) input_ctx->streams[0]->time_base.num /
                                             (float) input_ctx->streams[0]->time_base.den);
                    printf("\ndts_v=%d\npts_v=%d\n", dts_v, pts_v);
                    printf("\ndts_a=%d\npts_a=%d\n", dts_a, pts_a);
                    pts_v_temp += pts_v;
//pts_v_temp+=512;
                    dts_v_temp = pts_v_temp;
                    pts_a_temp += pts_a;
                    pts_a_temp += 1024;
                    dts_a_temp = pts_a_temp;
                    printf("\ndts_v=%d\npts_v=%d\n", dts_v, pts_v);
                    printf("\ndts_a=%d\npts_a=%d\n", dts_a, pts_a);
                } else {
                    dts_a = pts_a =
                            vedioDuraTime / ((float) input_ctx->streams[1]->time_base.num /
                                             (float) input_ctx->streams[1]->time_base.den);
                    printf("\nnonononodts_a=%d\npts_a=%d\n", dts_a, pts_a);
                    pts_v_temp += pts_v;
                    pts_v_temp += 1024;
                    dts_v_temp = pts_v_temp;
                    pts_a_temp += pts_a;
                    pts_a_temp += 2048;
                    dts_a_temp = pts_a_temp;
                    printf("\nnonononodts_a=%d\npts_a=%d\n", dts_a, pts_a);
                }
                vedioIndex++;
                input_ctx = ifmt_ctx_array[vedioIndex];
                continue;
            }
            break;
        }
        in_stream = input_ctx->streams[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];

        if (pkt.stream_index == 0) {
            printf("\npkt.pts=%lld\npkt.dts=%lld\n", pkt.pts, pkt.dts);
            {
                pts_v = pkt.pts;
                dts_v = pkt.dts;
                printf("\n============pts_v=%d\ndts_v=%d\n", pts_v, dts_v);
            }
            if (input_ctx != ifmt_ctx_array[0]) {
                pkt.pts += pts_v_temp;
                pkt.dts += dts_v_temp;
                printf("\n============pts_v=%d\ndts_v=%d\n", pts_v, dts_v);
            }
        } else if (pkt.stream_index == 1) {
            {
                pts_a = pkt.pts;
                dts_a = pkt.dts;
                printf("\n============pts_a=%d\ndts_a=%d\n", pts_a, dts_a);
            }
            if (input_ctx != ifmt_ctx_array[0]) {
                pkt.pts += pts_a_temp;
                pkt.dts += dts_a_temp;
                printf("\n============pts_a=%d\ndts_a=%d\n", pts_a, dts_a);
            }
        }

//        printf("\npkt.pts=%lld\npkt.dts=%lld\n",pkt.pts,pkt.dts);
//Convert PTS/DTS
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base,
                                   (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base,
                                   (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
//pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
//Write
        if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
            printf("Error muxing packet\n");
//break;
        }
        printf("Write %8d frames to output file\n", frame_index);
        av_free_packet(&pkt);
        frame_index++;
    }

//Write file trailer
    av_write_trailer(ofmt_ctx);
    end:
    for (int i = 0; i < count; i++) {
        avformat_close_input(&ifmt_ctx_array[i]);
    }

/* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);

}