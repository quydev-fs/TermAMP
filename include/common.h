#ifndef COMMON_H
#define COMMON_H

#include <gtk/gtk.h>
#include <string>
#include <vector>
#include <atomic>

// Winamp Aesthetic Constants
#define WINAMP_BG_COLOR "#282828"
#define WINAMP_FG_COLOR "#00E200"
#define WINAMP_BTN_COLOR "#454545"

enum RepeatMode {
    REP_OFF = 0,
    REP_ONE = 1,
    REP_ALL = 2
};

struct AppState {
    // Playback State
    bool playing = false;
    bool paused = false;
    
    // Advanced Playback
    bool shuffle = false;
    int repeatMode = REP_OFF;
    
    // Audio Data
    std::vector<std::string> playlist;
    std::vector<size_t> play_order; 
    int current_track_idx = -1;     
    double volume = 1.0;
    
    // NEW: Metadata Storage
    std::string current_track_name = "Ready"; 
};

#endif
