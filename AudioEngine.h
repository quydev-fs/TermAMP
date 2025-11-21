#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include "AppState.h"
#include <pthread.h>

class AudioEngine {
public:
    AudioEngine(AppState* state);
    ~AudioEngine();
    void start();
    void stop();

private:
    static void* threadEntry(void* arg);
    void audioLoop();

    AppState* app;
    pthread_t threadId;
};

#endif
