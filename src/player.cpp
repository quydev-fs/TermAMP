#include "player.h"
#include "visualizer.h"
#include <mpg123.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <unistd.h>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <numeric> // Added for iota fix

Player::Player(AppState* state) : app(state) {
    mpg123_init();
}

Player::~Player() {
    stop();
    mpg123_exit();
}

void Player::start() {
    pthread_create(&threadId, NULL, threadEntry, this);
}

void Player::stop() {
    pthread_join(threadId, NULL);
}

void* Player::threadEntry(void* arg) {
    ((Player*)arg)->audioLoop();
    return NULL;
}

void Player::audioLoop() {
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

        // --- FAIL-SAFE: Ensure Play Order exists ---
        if (app->play_order.size() != app->playlist.size()) {
            std::cout << "Fixing Play Order Sync..." << std::endl;
            app->play_order.resize(app->playlist.size());
            std::iota(app->play_order.begin(), app->play_order.end(), 0);
        }

        // Get actual file index from the shuffle map
        size_t actual_file_index = app->track_idx;
        if (app->track_idx < app->play_order.size()) {
            actual_file_index = app->play_order[app->track_idx];
        }

        std::string current_file = app->playlist[actual_file_index];

        if (mpg123_open(mh, current_file.c_str()) != MPG123_OK) {
            std::cerr << "Failed to open: " << current_file << std::endl;
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

        // Metadata
        mpg123_id3v1* v1; mpg123_id3v2* v2;
        if (mpg123_id3(mh, &v1, &v2) == MPG123_OK) {
            if (v2 && v2->title) app->current_title = v2->title->p;
            else if (v1) app->current_title = v1->title;
            else app->current_title = current_file;
        } else {
            app->current_title = current_file;
        }

        while (app->playing && app->running) {
             if (app->seek_request) {
                off_t offset = (off_t)(app->seek_pos * app->total_frames);
                mpg123_seek_frame(mh, offset, SEEK_SET);
                app->seek_request = false;
            }

            if (app->paused) { usleep(50000); continue; }

            int ret = mpg123_read(mh, buffer, buff_size, &done);
            
            // --- END OF TRACK LOGIC ---
            if (ret == MPG123_DONE) {
                size_t next = app->track_idx + 1;
                
                // Check against list size
                if (next >= app->playlist.size()) {
                    if (app->repeat) {
                        next = 0; 
                    } else {
                        app->playing = false; 
                        app->track_idx = 0;   
                        break;
                    }
                }
                app->track_idx = next;
                break; // Load new file
            }

            mpg123_frameinfo fi;
            if(mpg123_info(mh, &fi) == MPG123_OK) app->bitrate = fi.bitrate;
            app->current_frame = mpg123_tell(mh);

            // FFT
            int16_t* samples = (int16_t*)buffer;
            int sample_count = done / 2;
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

            float vol = (float)app->volume / 100.0f;
            vol = vol * vol * vol; 
            for(int i=0; i<sample_count; i++) samples[i] = (int16_t)(samples[i] * vol);

            if (pa) pa_simple_write(pa, buffer, done, &err);
        }
    }
    if (pa) pa_simple_free(pa);
    mpg123_delete(mh);
}
