
#ifndef FFMPEGDEMO_SPLITVIDEO_H
#define FFMPEGDEMO_SPLITVIDEO_H

#endif //FFMPEGDEMO_SPLITVIDEO_H

extern "C" {
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
};
bool executeSplit(unsigned int splitSeconds);
//param startSecond 开始时间 单位秒
//param endSecond 结束时间
bool executeSplitOneClip(unsigned int startSecond, unsigned int endSecond);