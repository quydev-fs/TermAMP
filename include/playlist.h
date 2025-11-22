#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "common.h"
#include <string>

// Loads files into the AppState playlist vector
void loadPlaylist(AppState& app, int argc, char** argv);

// Saves the current playlist to an M3U file
// Returns true if successful, false otherwise
bool savePlaylist(const AppState& app, std::string filename = "");

#endif

