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
    
    void setVolume(double volume);
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
    
    // Internal helper to extract tags
    void handleTags(GstTagList* tags);
};

#endif
