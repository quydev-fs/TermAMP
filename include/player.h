#ifndef PLAYER_H
#define PLAYER_H

#include "common.h"
#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <functional>

typedef void (*EOSCallback)(void* user_data);

class Player {
public:
    Player(AppState* state);
    ~Player();

    void load(const std::string& uri);
    void play();
    void pause();
    void stop();

    GstState getState();

    void setVolume(double volume);
    void seek(double seconds);
    double getPosition();
    double getDuration();

    // Equalizer functions
    void setupEqualizer();
    void setEqualizerBand(int band, double value);
    void setEqualizerBands(const std::vector<double>& bands);
    void enableEqualizer(bool enabled);
    bool isEqualizerEnabled() const;

    // Crossfading functions
    void startCrossfade(const std::string& nextUri);
    void stopCrossfade();
    bool isCrossfading() const;
    gboolean updateCrossfade();  // Main crossfade update method that will be called from the timer

    void setEOSCallback(EOSCallback cb, void* data);
    static gboolean busCallback(GstBus* bus, GstMessage* msg, gpointer data);

private:
    AppState* app;
    GstElement* pipeline;
    GstElement* equalizer;  // Audio equalizer element

    // For crossfading
    GstElement* next_pipeline;
    std::string next_uri;
    guint crossfade_timeout_id;

    // Static callback for crossfading (needs to be declared here)
    static gboolean crossfadeTimer(gpointer data);

    EOSCallback onEOS = nullptr;
    void* eosData = nullptr;

    void handleTags(GstTagList* tags);

    // Fix: Guard against spurious EOS signals on resume
    guint64 last_play_time = 0;
};

#endif
