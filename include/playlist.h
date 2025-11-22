#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "common.h"
#include <string>

void loadPlaylist(AppState& app, int argc, char** argv);
bool savePlaylist(const AppState& app, std::string filename = "");

// Re-calculates the play_order vector
void toggleShuffle(AppState& app);

#endif
