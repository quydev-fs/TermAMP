#ifndef PLAYER_H
#define PLAYER_H

#include "common.h"
#include <gst/gst.h>

class Player {
public:
    Player(AppState* state);
    ~Player();

    void load(const std::string& uri);
    void play();
    void pause();
    void stop();
    void setVolume(double vol); // Not implemented in UI yet, but good to have
    
    // NEW: Returns current position in seconds
    double getPosition(); 

    static gboolean busCallback(GstBus* bus, GstMessage* msg, gpointer data);

private:
    AppState* app;
    GstElement* pipeline;
};

#endif
