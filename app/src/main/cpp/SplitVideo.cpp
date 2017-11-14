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
out_stream->codec->codec_tag = 0;
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
if (ret < 0){
return false;
}
return true;

1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
35
36
帧拷贝

//param splitSeconds 为视频分割的时长
bool executeSplit(unsigned int splitSeconds)
{
    AVPacket readPkt, splitKeyPacket;
    int ret;
    av_register_all();
    if ((ret = avformat_open_input(&ifmtCtx, inputFileName.c_str(), 0, 0)) < 0) {
        return false;
    }

    if ((ret = avformat_find_stream_info(ifmtCtx, 0)) < 0) {
        return false;
    }
    for (int i = 0; i < ifmtCtx->nb_streams; i++) {

        AVStream *in_stream = ifmtCtx->streams[i];
        if (in_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            video_index = i;
        }

    }
    int den = ifmtCtx->streams[video_index]->r_frame_rate.den;
    int num = ifmtCtx->streams[video_index]->r_frame_rate.num;
    float fps = (float)num / den;
    unsigned int splitVideoSize = fps*splitSeconds;
    string save_name;
    save_name = outputFileName.substr(0, outputFileName.find_last_of("."));
    string temp_name = save_name + "0"+suffixName;
    avformat_alloc_output_context2(&ofmtCtx, NULL, NULL, temp_name.c_str());
    if (!ofmtCtx) {
        return false;
    }
    if (!writeVideoHeader(ifmtCtx, ofmtCtx, temp_name))
    {
        return false;
    }
    vector<uint64_t> vecKeyFramePos;
    uint64_t frame_index = 0;
    uint64_t keyFrame_index = 0;
    int frameCount = 0;
    //读取分割点附近的关键帧位置
    while (1)
    {
        ++frame_index;
        ret = av_read_frame(ifmtCtx, &readPkt);
        if (ret < 0)
        {
            break;
        }
        //过滤，只处理视频流
        if (readPkt.stream_index == video_index){

            ++frameCount;
            if (readPkt.flags&AV_PKT_FLAG_KEY)
            {
                keyFrame_index = frame_index;
            }
            if (frameCount>splitVideoSize)
            {
                vecKeyFramePos.push_back(keyFrame_index);
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
    if (vecKeyFramePos.empty()){
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
    while (1)
    {
        ++frame_index;
        ret = av_read_frame(ifmtCtx, &readPkt);
        if (ret < 0)
        {
            break;
        }

        av_packet_rescale_ts(&readPkt, ifmtCtx->streams[readPkt.stream_index]->time_base, ofmtCtx->streams[readPkt.stream_index]->time_base);
        prePts = readPkt.pts;
        preDts = readPkt.dts;
        readPkt.pts -= lastPts;
        readPkt.dts -= lastDts;
        if (readPkt.pts < readPkt.dts)
        {
            readPkt.pts = readPkt.dts + 1;
        }
        //为分割点处的关键帧要进行拷贝
        if (readPkt.flags&AV_PKT_FLAG_KEY&&frame_index == keyFrame_index)
        {
            av_copy_packet(&splitKeyPacket, &readPkt);
        }
        else{
            ret = av_interleaved_write_frame(ofmtCtx, &readPkt);
            if (ret < 0) {
                //break;

            }
        }

        if (frame_index == keyFrame_index)
        {
            lastDts = preDts;
            lastPts = prePts;
            if (keyFrameIter != vecKeyFramePos.end())
            {
                keyFrame_index = *keyFrameIter;
                ++keyFrameIter;
            }
            av_write_trailer(ofmtCtx);
            avio_close(ofmtCtx->pb);
            avformat_free_context(ofmtCtx);
            ++number;
            char num[10];
            _itoa_s(number, num, 10);
            temp_name = save_name + num + suffixName;
            avformat_alloc_output_context2(&ofmtCtx, NULL, NULL, temp_name.c_str());
            if (!ofmtCtx) {
                return false;
            }
            if (!writeVideoHeader(ifmtCtx, ofmtCtx, save_name + num + suffixName))
            {
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

