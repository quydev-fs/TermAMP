#include "playlist.h"
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream>

void loadPlaylist(AppState& app, int argc, char** argv) {
    if (argc < 2) return;

    struct stat s;
    if (stat(argv[1], &s) == 0) {
        if (s.st_mode & S_IFDIR) {
            DIR *dir; struct dirent *ent;
            if ((dir = opendir(argv[1])) != NULL) {
                while ((ent = readdir(dir)) != NULL) {
                    std::string fname = ent->d_name;
                    if(fname.length() > 4 && fname.substr(fname.length()-4) == ".mp3") {
                         app.playlist.push_back(std::string(argv[1]) + "/" + fname);
                    }
                }
                closedir(dir);
            }
        } else {
            std::string arg = argv[1];
            if (arg.substr(arg.length()-4) == ".m3u") {
                FILE* f = fopen(arg.c_str(), "r");
                char line[256];
                while(fgets(line, sizeof(line), f)) {
                    if(line[0] != '#') {
                        std::string l = line;
                        l.erase(std::remove(l.begin(), l.end(), '\n'), l.end());
                        if(!l.empty()) app.playlist.push_back(l);
                    }
                }
                fclose(f);
            } else {
                for(int i=1; i<argc; i++) app.playlist.push_back(argv[i]);
            }
        }
    }
    std::sort(app.playlist.begin(), app.playlist.end());
}

bool savePlaylist(const AppState& app, std::string filename) {
    if (app.playlist.empty()) return false;

    // Generate filename if empty: playlist_YYYY-MM-DD_HHMMSS.m3u
    if (filename.empty()) {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << "playlist_" << std::put_time(&tm, "%Y-%m-%d_%H%M%S") << ".m3u";
        filename = oss.str();
    }

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << " for writing." << std::endl;
        return false;
    }

    file << "#EXTM3U" << std::endl;
    
    for (const auto& track : app.playlist) {
        // Note: To get specific metadata (Duration/Title) here efficiently without 
        // re-parsing every file, we stick to basic M3U (paths only) which is standard.
        file << track << std::endl;
    }

    file.close();
    std::cout << "Playlist saved to: " << filename << std::endl;
    return true;
}
