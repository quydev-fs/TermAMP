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

    // NEW: Equalizer State
    bool eq_enabled = false;
    std::vector<double> eq_bands;

    // NEW: Crossfading State
    bool crossfading_enabled = false;
    double crossfade_duration = 3.0;  // 3 seconds default crossfade
    bool is_crossfading = false;

    // NEW: Audio conversion state
    bool conversion_in_progress = false;
    std::string last_conversion_output_path;
    double conversion_progress = 0.0;  // 0.0 to 1.0

    // NEW: Album art state
    std::string current_album_art_path;
    GdkPixbuf* current_album_art = nullptr;

    // Album art update callback
    void (*onAlbumArtChanged)(void* userData) = nullptr;
    void* albumArtCallbackUserData = nullptr;
};

#endif
