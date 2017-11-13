//
// Created by Administrator on 2017/11/12.
//
#include <android/log.h>

#ifndef FFMPEGDEMO_LOG_H
#define FFMPEGDEMO_LOG_H
#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"jason",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"jason",FORMAT,##__VA_ARGS__);

#endif //FFMPEGDEMO_LOG_H
