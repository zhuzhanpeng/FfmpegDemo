#include "SplitVideo.h"
#include "log.h"
#include <string>
#include <vector>
#include<cstdlib>

using namespace std;

bool writeVideoHeader(AVFormatContext *ifmtCtx, AVFormatContext *ofmtCtx, string out_filename) {
    AVOutputFormat *ofmt = NULL;
    int ret;
    ofmt = ofmtCtx->oformat;
    for (int i = 0; i < ifmtCtx->nb_streams; i++) {

//根据输入流创建输出流
        AVStream *in_stream = ifmtCtx->streams[i];

        AVStream *out_stream = avformat_new_stream(ofmtCtx, in_stream->codec->codec);
        if (!out_stream) {
            return false;
        }
//复制AVCodecContext的设置
        ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
        if (ret < 0) {
            return false;
        }
        out_stream->codec->
                codec_tag = 0;
        if (ofmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
            out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;


    }
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmtCtx->pb, out_filename.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            return false;
        }
    }
    ret = avformat_write_header(ofmtCtx, NULL);
    if (ret < 0) {
        return false;
    }
    return true;
}

//param splitSeconds 为视频分割的时长
bool executeSplit(unsigned int splitSeconds) {
    int video_index;
    av_register_all();
    avformat_network_init();
    AVFormatContext *ifmtCtx = avformat_alloc_context();
    AVFormatContext *ofmtCtx;
//    string inputFileName = "http://img.paas.onairm.cn/8abcbfaad1e48708b94466e024e24629base?avvod/m3u8/s/640*960/vb/400k/autosave/1";
    string inputFileName = "/sdcard/input.mp4";
    string outputFileName = "/sdcard/ts.mp4";
    string suffixName = ".mp4";
    AVPacket readPkt, splitKeyPacket;
    int ret;
    if (avformat_open_input(&ifmtCtx, inputFileName.c_str(), 0, 0) < 0) {
        LOGE("Could not open file.\n")
        return false;
    }

    if (avformat_find_stream_info(ifmtCtx, 0) < 0) {
        LOGE("Fail to find stream information\n")
        return false;
    }

    for (int i = 0; i < ifmtCtx->nb_streams; i++) {

        AVStream *in_stream = ifmtCtx->streams[i];
        if (in_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
        }
    }
    int den = ifmtCtx->streams[video_index]->r_frame_rate.den;
    int num = ifmtCtx->streams[video_index]->r_frame_rate.num;
    float fps = (float) num / den;
    LOGE("帧率%f", fps)
    unsigned int splitVideoSize = fps * splitSeconds;
    LOGE("一段视频的帧数%d", splitVideoSize);
    string save_name;
    save_name = outputFileName.substr(0, outputFileName.find_last_of("."));
    string temp_name = save_name + "0" + suffixName;
    avformat_alloc_output_context2(&ofmtCtx, NULL, NULL, temp_name.c_str());
    if (!ofmtCtx) {
        LOGE("Alloc out AVFormatContext failing.\n");
        return false;
    }
    if (!writeVideoHeader(ifmtCtx, ofmtCtx, temp_name)) {
        return false;
    }
    vector<uint64_t> vecKeyFramePos;
    uint64_t frame_index = 0;
    uint64_t keyFrame_index = 0;
    int frameCount = 0;
    //读取分割点附近的关键帧位置
    while (1) {
        ++frame_index;
        ret = av_read_frame(ifmtCtx, &readPkt);
        if (ret < 0) {
            break;
        }
        //过滤，只处理视频流
        if (readPkt.stream_index == video_index) {

            ++frameCount;
            if (readPkt.flags & AV_PKT_FLAG_KEY) {
                keyFrame_index = frame_index;
            }
            if (frameCount > splitVideoSize) {
                vecKeyFramePos.push_back(keyFrame_index);
                LOGE("KeyFrame 索引%d", keyFrame_index);
                frameCount = 0;
            }
        }
        av_packet_unref(&readPkt);
    }

    avformat_close_input(&ifmtCtx);
    ifmtCtx = NULL;
    //为了重新获取avformatcontext
    if ((ret = avformat_open_input(&ifmtCtx, inputFileName.c_str(), 0, 0)) < 0) {
        return -1;
    }

    if ((ret = avformat_find_stream_info(ifmtCtx, 0)) < 0) {
        return -1;
    }
    int number = 0;
    av_init_packet(&splitKeyPacket);
    splitKeyPacket.data = NULL;
    splitKeyPacket.size = 0;
    //时长对应的帧数超过视频的总视频帧数，则拷贝完整视频
    if (vecKeyFramePos.empty()) {
        vecKeyFramePos.push_back(frame_index);
    }
    vector<uint64_t>::iterator keyFrameIter = vecKeyFramePos.begin();

    keyFrame_index = *keyFrameIter;
    ++keyFrameIter;
    frame_index = 0;
    int64_t lastPts = 0;
    int64_t lastDts = 0;
    int64_t prePts = 0;
    int64_t preDts = 0;
    while (1) {
        ++frame_index;
        ret = av_read_frame(ifmtCtx, &readPkt);
        if (ret < 0) {
            break;
        }
        LOGE("帧索引%lld",frame_index);
        av_packet_rescale_ts(&readPkt, ifmtCtx->streams[readPkt.stream_index]->time_base,
                             ofmtCtx->streams[readPkt.stream_index]->time_base);
        prePts = readPkt.pts;
        preDts = readPkt.dts;
        readPkt.pts -= lastPts;
        LOGE("readPkt.pts %lld lastPts %lld",readPkt.pts,lastPts);
        readPkt.dts -= lastDts;
        LOGE("readPkt.dts %lld lastDts %lld",readPkt.dts,lastDts);

        if (readPkt.pts < readPkt.dts) {
            readPkt.pts = readPkt.dts + 1;
        }
        //为分割点处的关键帧要进行拷贝
        if (readPkt.flags & AV_PKT_FLAG_KEY && frame_index == keyFrame_index) {
            av_copy_packet(&splitKeyPacket, &readPkt);
        } else {
            ret = av_interleaved_write_frame(ofmtCtx, &readPkt);
            if (ret < 0) {
                break;
            }
        }

        if (frame_index == keyFrame_index) {
            lastDts = preDts;
            lastPts = prePts;
            if (keyFrameIter != vecKeyFramePos.end()) {
                keyFrame_index = *keyFrameIter;
                ++keyFrameIter;
            }
            av_write_trailer(ofmtCtx);
            avio_close(ofmtCtx->pb);
            avformat_free_context(ofmtCtx);
            ++number;
            char num[10];
            sprintf(num, "%d", number);
            temp_name = save_name + num + suffixName;
            avformat_alloc_output_context2(&ofmtCtx, NULL, NULL, temp_name.c_str());
            if (!ofmtCtx) {
                return false;
            }
            if (!writeVideoHeader(ifmtCtx, ofmtCtx, temp_name)) {
                return false;
            }
            splitKeyPacket.pts = 0;
            splitKeyPacket.dts = 0;
            //把上一个分片处的关键帧写入到下一个分片的起始处，保证下一个分片的开头为I帧
            ret = av_interleaved_write_frame(ofmtCtx, &splitKeyPacket);
        }
        av_packet_unref(&readPkt);
    }
    av_packet_unref(&splitKeyPacket);
    av_write_trailer(ofmtCtx);
    avformat_close_input(&ifmtCtx);
    avio_close(ofmtCtx->pb);
    avformat_free_context(ofmtCtx);
    return true;
}

//param startSecond 开始时间 单位秒
//param endSecond 结束时间
bool executeSplitOneClip(unsigned int startSecond, unsigned int endSecond) {
    uint64_t start_keyFrame_pos = -1, end_keyFrame_pos = -1;
    uint64_t start_frame_pos = -1, end_frame_pos = -1;
    int video_index;
    av_register_all();
    avformat_network_init();
    AVFormatContext *ifmtCtx = avformat_alloc_context();
    AVFormatContext *ofmtCtx;
    string inputFileName = "/sdcard/input.mp4";
    string outputFileName = "/sdcard/ts.mp4";
    string suffixName = ".mp4";
    AVPacket readPkt;
    int ret;
    if (avformat_open_input(&ifmtCtx, inputFileName.c_str(), 0, 0) < 0) {
        LOGE("Could not open file.\n")
        return false;
    }

    if (avformat_find_stream_info(ifmtCtx, 0) < 0) {
        LOGE("Fail to find stream information\n")
        return false;
    }

    for (int i = 0; i < ifmtCtx->nb_streams; i++) {
        AVStream *in_stream = ifmtCtx->streams[i];
        if (in_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
        }
    }
    int den = ifmtCtx->streams[video_index]->r_frame_rate.den;//1
    int num = ifmtCtx->streams[video_index]->r_frame_rate.num;//25
    float fps = (float) num / den;
    LOGE("帧率%f", fps);
    start_frame_pos = fps * startSecond;
    end_frame_pos = fps * endSecond;
    LOGE("start_frame_pos 索引%lld", start_frame_pos);
    LOGE("end_frame_pos 索引%lld", end_frame_pos);
    string save_name;
    save_name = outputFileName.substr(0, outputFileName.find_last_of("."));
    string temp_name = save_name + "0" + suffixName;
    avformat_alloc_output_context2(&ofmtCtx, NULL, NULL, temp_name.c_str());
    if (!ofmtCtx) {
        LOGE("Alloc out AVFormatContext failing.\n");
        return false;
    }
    if (!writeVideoHeader(ifmtCtx, ofmtCtx, temp_name)) {
        return false;
    }

    uint64_t frame_index = 0;

    int frameCount = 0;
    //读取分割点附近的关键帧位置
    while (1) {
        ++frame_index;
        ret = av_read_frame(ifmtCtx, &readPkt);
        if (ret < 0) {
            break;
        }
        //过滤，只处理视频流
        if (readPkt.stream_index == video_index) {
            ++frameCount;
            if ((readPkt.flags & AV_PKT_FLAG_KEY) &&
                (frameCount >= start_frame_pos) &&
                (start_keyFrame_pos == -1)
                    ) {
                start_keyFrame_pos = frame_index;
                LOGE("start_keyFrame_pos 索引%lld", start_keyFrame_pos);
            }
            if ((readPkt.flags & AV_PKT_FLAG_KEY) &&
                (frameCount >= end_frame_pos) &&
                (end_keyFrame_pos == -1)
                    ) {
                end_keyFrame_pos = frame_index;
                LOGE("end_keyFrame_pos 索引%lld", end_keyFrame_pos);
            }

        }
        av_packet_unref(&readPkt);
    }


    avformat_close_input(&ifmtCtx);
    ifmtCtx = NULL;
    //为了重新获取avformatcontext
    if ((ret = avformat_open_input(&ifmtCtx, inputFileName.c_str(), 0, 0)) < 0) {
        return -1;
    }

    if ((ret = avformat_find_stream_info(ifmtCtx, 0)) < 0) {
        return -1;
    }
    //时长对应的帧数超过视频的总视频帧数，则拷贝完整视频
    /*
     * 此处为参数有效性检查，有待补充
     * */
    /*if (vecKeyFramePos.empty()) {
        vecKeyFramePos.push_back(frame_index);
    }*/

    frame_index = 0;
    int64_t lastPts = 0;
    int64_t lastDts = 0;
    int64_t prePts = 0;
    int64_t preDts = 0;

    int64_t write_frame_total=0;
    while (1) {
        ++frame_index;
        ret = av_read_frame(ifmtCtx, &readPkt);
        if (ret < 0) {
            break;
        }
        av_packet_rescale_ts(&readPkt, ifmtCtx->streams[readPkt.stream_index]->time_base,
                             ofmtCtx->streams[readPkt.stream_index]->time_base);

        prePts = readPkt.pts;
        preDts = readPkt.dts;
       /* readPkt.pts -= lastPts;
        readPkt.dts -= lastDts;*/

        if (readPkt.pts < readPkt.dts) {
            readPkt.pts = readPkt.dts + 1;
        }

        //为分割点处的关键帧要进行拷贝
        /*if(readPkt.flags & AV_PKT_FLAG_KEY && frame_index == start_keyFrame_pos){
            lastDts = preDts;
            lastPts = prePts;
        }*/

        if(frame_index>=start_keyFrame_pos && frame_index < end_keyFrame_pos){
            ret = av_interleaved_write_frame(ofmtCtx, &readPkt);
            write_frame_total++;
            LOGE("write_frame_total %lld",write_frame_total);
            if (ret < 0) {
                break;
            }
        }
        av_packet_unref(&readPkt);
    }
    av_write_trailer(ofmtCtx);
    avformat_close_input(&ifmtCtx);
    avio_close(ofmtCtx->pb);
    avformat_free_context(ofmtCtx);
    return true;
}



