#ifndef APPSTATE_H
#define APPSTATE_H

#include <atomic>
#include <vector>
#include <string>

// --- VISUAL CONSTANTS ---
const int W_WIDTH = 275;
const int W_HEIGHT = 116;
const unsigned long C_BG = 0x191919;
const unsigned long C_FACE = 0x282828;
const unsigned long C_BTN_L = 0x454545;
const unsigned long C_BTN_D = 0x151515;
const unsigned long C_TXT_GRN = 0x00E200;
const unsigned long C_TXT_YEL = 0xD2D200;
const unsigned long C_VIS_BG = 0x000000;
const unsigned long C_TITLE_BG = 0x000080; // Winamp Blue

struct AppState {
    std::atomic<bool> running{true};
    std::atomic<bool> playing{false};
    std::atomic<bool> paused{false};
    std::atomic<int> volume{90}; // 0-100

    // Audio Data
    std::vector<std::string> playlist;
    std::atomic<size_t> track_idx{0};
    std::atomic<long> current_frame{0};
    std::atomic<long> total_frames{0};
    std::atomic<int> sample_rate{44100};
    std::atomic<int> bitrate{128};
    
    // Seek control
    std::atomic<bool> seek_request{false};
    std::atomic<double> seek_pos{0.0}; // 0.0 to 1.0

    // Visualization Data (16 Bands)
    std::atomic<int> viz_bands[16]; 
    std::string current_title = "TermuxMusic95";
};

#endif
