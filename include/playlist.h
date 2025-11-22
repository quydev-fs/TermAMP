#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "common.h"
#include <string>

// File Operations
void loadPlaylist(AppState& app, int argc, char** argv);
bool savePlaylist(const AppState& app, std::string filename = "");
void clearPlaylist(AppState& app); // <--- NEW

// Playback Logic
void toggleShuffle(AppState& app);

// Navigation Logic
void playNext(AppState& app, bool forceChange = false);
void playPrevious(AppState& app);

#endif
