#ifndef PLAYER_H
#define PLAYER_H

#include "common.h"
#include <gst/gst.h>
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
    
    // --- NEW CONTROLS ---
    void setVolume(double volume); // 0.0 to 1.0
    void seek(double seconds);
    double getPosition();
    double getDuration();

    void setEOSCallback(EOSCallback cb, void* data);
    static gboolean busCallback(GstBus* bus, GstMessage* msg, gpointer data);

private:
    AppState* app;
    GstElement* pipeline;
    
    EOSCallback onEOS = nullptr;
    void* eosData = nullptr;
};

#endif
