#include "synconize.h"
#include <queue>

#include <libavformat/avformat.h>
extern "C" {
#include <pthread.h>
#include <unistd.h>
using namespace std;



void *audioSynVideo(const char *path) {
    pthread_t pid;
    pthread_create(&pid,NULL,fill_stack,path);
    int* ret=0;
    pthread_join(pid, (void **) &ret);
}

}



