
#ifndef FFMPEGDEMO_SPLITVIDEO_H
#define FFMPEGDEMO_SPLITVIDEO_H

#endif //FFMPEGDEMO_SPLITVIDEO_H

extern "C" {
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
};
bool executeSplit(unsigned int splitSeconds);
