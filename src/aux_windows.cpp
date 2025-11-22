#include "aux_windows.h"
#include <X11/Xutil.h>
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <limits.h>

// Helper shared by aux windows
void drawAuxBevel(Display* dpy, Window win, GC gc, int x, int y, int w, int h, bool sunken) {
    int sx = x * UI_SCALE;
    int sy = y * UI_SCALE;
    int sw = w * UI_SCALE;
    int sh = h * UI_SCALE;
    int th = std::max(1, UI_SCALE / 2);

    XSetForeground(dpy, gc, sunken ? C_BTN_D : C_BTN_L);
    XFillRectangle(dpy, win, gc, sx, sy, sw - th, th);
    XFillRectangle(dpy, win, gc, sx, sy, th, sh - th);

    XSetForeground(dpy, gc, sunken ? C_BTN_L : C_BTN_D);
    XFillRectangle(dpy, win, gc, sx + sw - th, sy, th, sh);
    XFillRectangle(dpy, win, gc, sx, sy + sh - th, sw, th);
}

// ================= PLAYLIST VIEWER =================

PlaylistViewer::PlaylistViewer(AppState* state, Display* d) : app(state), dpy(d) {
    int s = DefaultScreen(dpy);
    // Width 275, Height based on items
    win = XCreateSimpleWindow(dpy, RootWindow(dpy, s), 20, 150, 
                              275 * UI_SCALE, 150 * UI_SCALE, 
                              1, BlackPixel(dpy, s), C_FACE);
    XSelectInput(dpy, win, ExposureMask | ButtonPressMask);
    XStoreName(dpy, win, "Playlist Editor");
    gc = XCreateGC(dpy, win, 0, NULL);
}

PlaylistViewer::~PlaylistViewer() {
    XDestroyWindow(dpy, win);
}

void PlaylistViewer::show() {
    XMapWindow(dpy, win);
    visible = true;
    render();
}

void PlaylistViewer::hide() {
    XUnmapWindow(dpy, win);
    visible = false;
}

void PlaylistViewer::render() {
    if(!visible) return;
    
    // BG
    XSetForeground(dpy, gc, C_FACE);
    XFillRectangle(dpy, win, gc, 0, 0, 275*UI_SCALE, 150*UI_SCALE);

    // Title Bar
    XSetForeground(dpy, gc, C_TITLE_BG);
    XFillRectangle(dpy, win, gc, 0, 0, 275*UI_SCALE, 14*UI_SCALE);
    drawAuxBevel(dpy, win, gc, 0, 0, 275, 14, false);
    
    XSetForeground(dpy, gc, 0xFFFFFF);
    std::string title = "Playlist (" + std::to_string(app->playlist.size()) + ")";
    XDrawString(dpy, win, gc, 5*UI_SCALE, 11*UI_SCALE, title.c_str(), title.length());

    // Close Button [X]
    drawAuxBevel(dpy, win, gc, 260, 2, 10, 10, false);
    XDrawString(dpy, win, gc, 263*UI_SCALE, 10*UI_SCALE, "X", 1);

    // List Area
    XSetForeground(dpy, gc, C_VIS_BG);
    XFillRectangle(dpy, win, gc, 5*UI_SCALE, 20*UI_SCALE, 265*UI_SCALE, 120*UI_SCALE);
    drawAuxBevel(dpy, win, gc, 4, 19, 267, 122, true);

    // Draw Items
    int y = 32;
    int itemH = 12;
    
    for(int i = 0; i < maxVisibleItems; i++) {
        int idx = scrollOffset + i;
        if(idx >= app->playlist.size()) break;

        std::string fullpath = app->playlist[idx];
        // Extract filename
        size_t lastSlash = fullpath.find_last_of("/");
        std::string name = (lastSlash != std::string::npos) ? fullpath.substr(lastSlash + 1) : fullpath;
        
        // Highlight playing track
        if(idx == app->track_idx) XSetForeground(dpy, gc, C_TXT_GRN);
        else XSetForeground(dpy, gc, 0xFFFFFF); // White for others

        if (name.length() > 40) name = name.substr(0, 37) + "...";
        
        std::string line = std::to_string(idx+1) + ". " + name;
        XDrawString(dpy, win, gc, 8*UI_SCALE, y*UI_SCALE, line.c_str(), line.length());
        y += itemH;
    }
}

void PlaylistViewer::handleInput(int x, int y) {
    // Scale down
    x /= UI_SCALE;
    y /= UI_SCALE;

    // Close Button
    if (y >= 2 && y <= 12 && x >= 260 && x <= 270) {
        hide();
        return;
    }

    // Click on list item
    if (y >= 20 && y <= 140) {
        int itemH = 12;
        int clickedIndex = scrollOffset + ((y - 32) / itemH) + 1; // Adjustment
        
        if (clickedIndex >= scrollOffset && clickedIndex < scrollOffset + maxVisibleItems) {
            if (clickedIndex < app->playlist.size()) {
                app->track_idx = clickedIndex;
                app->playing = true;
                app->paused = false;
                app->seek_pos = 0.0;
                app->seek_request = false;
                render();
            }
        }
    }
    
    // Simple Scroll zones (Top/Bottom of list area)
    if (x > 250 && y > 20 && y < 60) handleScroll(-1);
    if (x > 250 && y > 100 && y < 140) handleScroll(1);
}

void PlaylistViewer::handleScroll(int direction) {
    scrollOffset += direction;
    if (scrollOffset < 0) scrollOffset = 0;
    if (scrollOffset > (int)app->playlist.size() - maxVisibleItems) 
        scrollOffset = std::max(0, (int)app->playlist.size() - maxVisibleItems);
    render();
}

// ================= FILE BROWSER =================

FileBrowser::FileBrowser(AppState* state, Display* d) : app(state), dpy(d) {
    int s = DefaultScreen(dpy);
    win = XCreateSimpleWindow(dpy, RootWindow(dpy, s), 30, 30, 
                              275 * UI_SCALE, 200 * UI_SCALE, 
                              1, BlackPixel(dpy, s), C_FACE);
    XSelectInput(dpy, win, ExposureMask | ButtonPressMask);
    XStoreName(dpy, win, "Add Files");
    gc = XCreateGC(dpy, win, 0, NULL);
    
    // Start at /sdcard or /
    currentPath = "/data/data/com.termux/files/home";
    struct stat info;
    if(stat(currentPath.c_str(), &info) != 0) currentPath = "/sdcard/download"; // Fallback
    
    refreshList();
}

FileBrowser::~FileBrowser() {
    XDestroyWindow(dpy, win);
}

void FileBrowser::show() {
    XMapWindow(dpy, win);
    visible = true;
    refreshList();
    render();
}

void FileBrowser::hide() {
    XUnmapWindow(dpy, win);
    visible = false;
}

void FileBrowser::refreshList() {
    entries.clear();
    DIR *dir;
    struct dirent *ent;
    
    // Add ".."
    if (currentPath != "/") {
        entries.push_back({"..", true});
    }

    if ((dir = opendir(currentPath.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            std::string name = ent->d_name;
            if(name == "." || name == "..") continue;
            
            bool isDir = (ent->d_type == DT_DIR);
            bool isMusic = false;
            if (!isDir && name.length() > 4) {
                std::string ext = name.substr(name.length()-4);
                if (ext == ".mp3" || ext == ".m3u") isMusic = true;
            }

            if (isDir || isMusic) {
                entries.push_back({name, isDir});
            }
        }
        closedir(dir);
    }
    
    // Sort: Dir first, then alpha
    std::sort(entries.begin(), entries.end(), [](const FileEntry& a, const FileEntry& b) {
        if (a.isDir != b.isDir) return a.isDir > b.isDir;
        return a.name < b.name;
    });
    
    scrollOffset = 0;
}

void FileBrowser::navigateUp() {
    if (currentPath == "/") return;
    size_t lastSlash = currentPath.find_last_of("/");
    if (lastSlash == 0) currentPath = "/";
    else currentPath = currentPath.substr(0, lastSlash);
    refreshList();
    render();
}

void FileBrowser::navigateTo(const std::string& dir) {
    if (currentPath == "/") currentPath += dir;
    else currentPath += "/" + dir;
    refreshList();
    render();
}

void FileBrowser::loadFile(const std::string& filename) {
    std::string full = (currentPath == "/") ? "/" + filename : currentPath + "/" + filename;
    
    if (filename.substr(filename.length()-4) == ".m3u") {
        // Load M3U
        FILE* f = fopen(full.c_str(), "r");
        char line[256];
        while(fgets(line, sizeof(line), f)) {
            if(line[0] != '#') {
                std::string l = line;
                l.erase(std::remove(l.begin(), l.end(), '\n'), l.end());
                if(!l.empty()) app->playlist.push_back(l);
            }
        }
        fclose(f);
    } else {
        // Add File
        app->playlist.push_back(full);
    }
    hide(); // Auto close on select? Or stay open? Let's close.
}

void FileBrowser::render() {
    if(!visible) return;
    
    XSetForeground(dpy, gc, C_FACE);
    XFillRectangle(dpy, win, gc, 0, 0, 275*UI_SCALE, 200*UI_SCALE);

    // Header
    XSetForeground(dpy, gc, C_TITLE_BG);
    XFillRectangle(dpy, win, gc, 0, 0, 275*UI_SCALE, 14*UI_SCALE);
    drawAuxBevel(dpy, win, gc, 0, 0, 275, 14, false);
    
    XSetForeground(dpy, gc, 0xFFFFFF);
    XDrawString(dpy, win, gc, 5*UI_SCALE, 11*UI_SCALE, "Add Files...", 12);

    // Close [X]
    drawAuxBevel(dpy, win, gc, 260, 2, 10, 10, false);
    XDrawString(dpy, win, gc, 263*UI_SCALE, 10*UI_SCALE, "X", 1);

    // Path Display
    XSetForeground(dpy, gc, C_TXT_YEL);
    std::string pathDisp = currentPath;
    if(pathDisp.length() > 35) pathDisp = "..." + pathDisp.substr(pathDisp.length()-35);
    XDrawString(dpy, win, gc, 5*UI_SCALE, 25*UI_SCALE, pathDisp.c_str(), pathDisp.length());

    // List Area
    XSetForeground(dpy, gc, C_VIS_BG);
    XFillRectangle(dpy, win, gc, 5*UI_SCALE, 30*UI_SCALE, 265*UI_SCALE, 160*UI_SCALE);
    drawAuxBevel(dpy, win, gc, 4, 29, 267, 162, true);

    int y = 42;
    int itemH = 12;
    int maxItems = 12; // Taller window

    for(int i = 0; i < maxItems; i++) {
        int idx = scrollOffset + i;
        if(idx >= entries.size()) break;

        const auto& e = entries[idx];
        if (e.isDir) XSetForeground(dpy, gc, 0xFFD700); // Gold for dirs
        else XSetForeground(dpy, gc, C_TXT_GRN); // Green for files

        std::string name = (e.isDir ? "[" : "") + e.name + (e.isDir ? "]" : "");
        if(name.length() > 40) name = name.substr(0, 37) + "...";
        
        XDrawString(dpy, win, gc, 8*UI_SCALE, y*UI_SCALE, name.c_str(), name.length());
        y += itemH;
    }
}

void FileBrowser::handleInput(int x, int y) {
    x /= UI_SCALE;
    y /= UI_SCALE;

    // Close
    if (y >= 2 && y <= 12 && x >= 260 && x <= 270) {
        hide();
        return;
    }
    
    // Scroll Touch Zones (Right side of list)
    if (x > 220 && y > 30 && y < 190) {
        if (y < 110) handleScroll(-1);
        else handleScroll(1);
        return;
    }

    // List Selection
    if (y >= 30 && y <= 190) {
        int itemH = 12;
        int idx = scrollOffset + ((y - 42) / itemH) + 1;
        
        if (idx >= scrollOffset && idx < entries.size()) {
            const auto& e = entries[idx];
            if (e.name == "..") navigateUp();
            else if (e.isDir) navigateTo(e.name);
            else loadFile(e.name);
        }
    }
}

void FileBrowser::handleScroll(int direction) {
    scrollOffset += direction * 3; // scroll 3 items
    if (scrollOffset < 0) scrollOffset = 0;
    if (scrollOffset > (int)entries.size() - 12) 
        scrollOffset = std::max(0, (int)entries.size() - 12);
    render();
}
