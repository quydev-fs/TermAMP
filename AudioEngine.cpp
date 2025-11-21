#include "AudioEngine.h"
#include "FFT.h"
#include <mpg123.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <unistd.h>
#include <cmath>
#include <algorithm>
#include <iostream>

AudioEngine::AudioEngine(AppState* state) : app(state) {
    mpg123_init();
}

AudioEngine::~AudioEngine() {
    stop();
    mpg123_exit();
}

void AudioEngine::start() {
    pthread_create(&threadId, NULL, threadEntry, this);
}

void AudioEngine::stop() {
    pthread_join(threadId, NULL);
}

void* AudioEngine::threadEntry(void* arg) {
    ((AudioEngine*)arg)->audioLoop();
    return NULL;
}

void AudioEngine::audioLoop() {
    mpg123_handle* mh = mpg123_new(NULL, NULL);
    pa_simple* pa = NULL;
    const size_t buff_size = 8192;
    unsigned char buffer[buff_size];
    size_t done;

    while (app->running) {
        if (!app->playing || app->playlist.empty()) {
            usleep(50000);
            continue;
        }

        if (mpg123_open(mh, app->playlist[app->track_idx].c_str()) != MPG123_OK) {
            app->playing = false;
            continue;
        }

        long rate; int channels, enc;
        mpg123_getformat(mh, &rate, &channels, &enc);
        app->sample_rate = rate;
        app->total_frames = mpg123_length(mh);

        pa_sample_spec ss = { .format = PA_SAMPLE_S16LE, .rate = (uint32_t)rate, .channels = (uint8_t)channels };
        int err;
        if (pa) pa_simple_free(pa);
        pa = pa_simple_new(NULL, "TermuxMusic95", PA_STREAM_PLAYBACK, NULL, "Music", &ss, NULL, NULL, &err);

        // Update Metadata
        mpg123_id3v1* v1; mpg123_id3v2* v2;
        if (mpg123_id3(mh, &v1, &v2) == MPG123_OK) {
            if (v2 && v2->title) app->current_title = v2->title->p;
            else if (v1) app->current_title = v1->title;
            else app->current_title = app->playlist[app->track_idx];
        } else {
            app->current_title = app->playlist[app->track_idx];
        }

        // Playback Loop
        while (app->playing && app->running) {
             if (app->seek_request) {
                off_t offset = (off_t)(app->seek_pos * app->total_frames);
                mpg123_seek_frame(mh, offset, SEEK_SET);
                app->seek_request = false;
            }

            if (app->paused) { usleep(50000); continue; }

            int ret = mpg123_read(mh, buffer, buff_size, &done);
            if (ret == MPG123_DONE) {
                size_t next = app->track_idx + 1;
                if (next >= app->playlist.size()) next = 0;
                app->track_idx = next;
                break; 
            }

            mpg123_frameinfo fi;
            if(mpg123_info(mh, &fi) == MPG123_OK) app->bitrate = fi.bitrate;
            app->current_frame = mpg123_tell(mh);

            // FFT & Volume
            int16_t* samples = (int16_t*)buffer;
            int sample_count = done / 2;
            
            // FFT Analysis
            CArray data(512);
            for(int i=0; i<512 && i<sample_count; i++) data[i] = (double)samples[i];
            computeFFT(data);
            
            for(int i=0; i<16; i++) {
                double mag = std::abs(data[i * 2 + 1]); 
                int h = (int)(std::log(mag + 1) * 3.0); 
                if (h > 16) h = 16; if (h < 0) h = 0;
                if (h >= app->viz_bands[i]) app->viz_bands[i] = h;
                else app->viz_bands[i] = std::max(0, app->viz_bands[i] - 1);
            }

            // Volume Application
            float vol = (float)app->volume / 100.0f;
            vol = vol * vol * vol; 
            for(int i=0; i<sample_count; i++) samples[i] = (int16_t)(samples[i] * vol);

            if (pa) pa_simple_write(pa, buffer, done, &err);
        }
    }
    if (pa) pa_simple_free(pa);
    mpg123_delete(mh);
}
