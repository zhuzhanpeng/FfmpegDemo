//
// Created by Edison on 2017/12/13.
//

#include "merge_video.h"
#include "log.h"
/*
*最简单的视频合并
*缪国凯 Mickel
*821486004@qq.com
*本程序实现把2个视频合并为一个视频，不涉及编解码，但是对视频源有要求，必须是相同的参数
*着重理解第二个视频开始的时候的pts和dts计算
*注：只处理一个视频流和一个音频流，若流多了，估计会crash
*2015-5-14
*/


#ifdef __cplusplus
extern "C"
{
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswresample/swresample.h>
#include <libavutil/fifo.h>
#include <libavutil/audio_fifo.h>

#ifdef __cplusplus
};
#endif

AVFormatContext *in1_fmtctx = NULL, *in2_fmtctx = NULL, *out_fmtctx = NULL;
AVStream *out_video_stream = NULL, *out_audio_stream = NULL;
int video_stream_index = -1, audio_stream_index = -1;


int open_input(const char *in1_name, const char *in2_name) {
    int ret = -1;
    if ((ret = avformat_open_input(&in1_fmtctx, in1_name, NULL, NULL)) < 0) {
       LOGE("can not open the first input context!\n");
        return ret;
    }
    if ((ret = avformat_find_stream_info(in1_fmtctx, NULL)) < 0) {
        LOGE("can not find the first input stream info!\n");
        return ret;
    }

    if ((ret = avformat_open_input(&in2_fmtctx, in2_name, NULL, NULL)) < 0) {
        LOGE("can not open the first input context!\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(in2_fmtctx, NULL)) < 0) {
        LOGE("can not find the second input stream info!\n");
        return ret;
    }
    return 1;
}

int open_output(const char *out_name) {
    int ret = -1;
    if ((ret = avformat_alloc_output_context2(&out_fmtctx, NULL, NULL, out_name)) < 0) {
        LOGE("can not alloc context for output!\n");
        return ret;
    }

    //new stream for out put
    for (int i = 0; i < in1_fmtctx->nb_streams; i++) {
        if (in1_fmtctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            out_video_stream = avformat_new_stream(out_fmtctx, NULL);
            if (!out_video_stream) {
                LOGE("Failed allocating output1 video stream\n");
                ret = AVERROR_UNKNOWN;
                return ret;
            }
            if ((ret = avcodec_copy_context(out_video_stream->codec,
                                            in1_fmtctx->streams[i]->codec)) < 0) {
                printf("can not copy the video codec context!\n");
                return ret;
            }
            out_video_stream->codec->codec_tag = 0;
            if (out_fmtctx->oformat->flags & AVFMT_GLOBALHEADER) {
                out_video_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
            }
        } else if (in1_fmtctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            out_audio_stream = avformat_new_stream(out_fmtctx, NULL);

            if (!out_audio_stream) {
                LOGE("Failed allocating output1 video stream\n");
                ret = AVERROR_UNKNOWN;
                return ret;
            }
            if ((ret = avcodec_copy_context(out_audio_stream->codec,
                                            in1_fmtctx->streams[i]->codec)) < 0) {
                LOGE("can not copy the video codec context!\n");
                return ret;
            }
            out_audio_stream->codec->codec_tag = 0;
            if (out_fmtctx->oformat->flags & AVFMT_GLOBALHEADER) {
                out_audio_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
            }
        }
    }

    //open output file
    if (!(out_fmtctx->oformat->flags & AVFMT_NOFILE)) {
        if ((ret = avio_open(&out_fmtctx->pb, out_name, AVIO_FLAG_WRITE)) < 0) {
            LOGE("can not open the out put file handle!\n");
            return ret;
        }
    }

    //write out  file header
    if ((ret = avformat_write_header(out_fmtctx, NULL)) < 0) {
        LOGE("Error occurred when opening video output file\n");
        return ret;
    }
    return 1;
}

int merge(const char* input_file1,const char* input_file2,const char* out_name) {
    av_register_all();
    int ret=0;

    if ((ret=open_input(input_file1, input_file2))<0) {
        LOGE("ret%d",ret);
//        goto end;
    }

    if ((ret=open_output(out_name))<0) {
        LOGE("out%d",ret);
//        goto end;
    }
    LOGE("执行了吗");

    AVFormatContext *input_ctx = in1_fmtctx;
    AVPacket pkt;
    int pts_v, pts_a, dts_v, dts_a;
    while (1) {
        if (0 > av_read_frame(input_ctx, &pkt)) {
            if (input_ctx == in1_fmtctx) {
                float vedioDuraTime, audioDuraTime;

                //calc the first media dura time
                vedioDuraTime = ((float) input_ctx->streams[video_stream_index]->time_base.num /
                                 (float) input_ctx->streams[video_stream_index]->time_base.den) *
                                ((float) pts_v);
                audioDuraTime = ((float) input_ctx->streams[audio_stream_index]->time_base.num /
                                 (float) input_ctx->streams[audio_stream_index]->time_base.den) *
                                ((float) pts_a);

                //calc the pts and dts end of the first media
                if (audioDuraTime > vedioDuraTime) {
                    dts_v = pts_v = audioDuraTime /
                                    ((float) input_ctx->streams[video_stream_index]->time_base.num /
                                     (float) input_ctx->streams[video_stream_index]->time_base.den);
                    dts_a++;
                    pts_a++;
                } else {
                    dts_a = pts_a = vedioDuraTime /
                                    ((float) input_ctx->streams[audio_stream_index]->time_base.num /
                                     (float) input_ctx->streams[audio_stream_index]->time_base.den);
                    dts_v++;
                    pts_v++;
                }
                input_ctx = in2_fmtctx;
                continue;
            }
            break;
        }

        if (pkt.stream_index == video_stream_index) {
            if (input_ctx == in2_fmtctx) {
                pkt.pts += pts_v;
                pkt.dts += dts_v;
            } else {
                pts_v = pkt.pts;
                dts_v = pkt.dts;
            }
        } else if (pkt.stream_index == audio_stream_index) {
            if (input_ctx == in2_fmtctx) {
                pkt.pts += pts_a;
                pkt.dts += dts_a;
            } else {
                pts_a = pkt.pts;
                dts_a = pkt.dts;
            }
        }

        pkt.pts = av_rescale_q_rnd(pkt.pts, input_ctx->streams[pkt.stream_index]->time_base,
                                   out_fmtctx->streams[pkt.stream_index]->time_base,
                                   (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, input_ctx->streams[pkt.stream_index]->time_base,
                                   out_fmtctx->streams[pkt.stream_index]->time_base,
                                   (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.pos = -1;

        if (av_interleaved_write_frame(out_fmtctx, &pkt) < 0) {
            printf("Error muxing packet\n");
            //break;
        }
        av_free_packet(&pkt);
    }

    av_write_trailer(out_fmtctx);

    end:
    avformat_close_input(&in1_fmtctx);
    avformat_close_input(&in2_fmtctx);

    /* close output */
    if (out_fmtctx && !(out_fmtctx->oformat->flags & AVFMT_NOFILE))
        avio_close(out_fmtctx->pb);

    avformat_free_context(out_fmtctx);

    return 0;
}
