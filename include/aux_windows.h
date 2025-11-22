#ifndef AUX_WINDOWS_H
#define AUX_WINDOWS_H

#include "common.h"
#include <X11/Xlib.h>
#include <vector>
#include <string>

// Helper for drawing shared styles
void drawAuxBevel(Display* dpy, Window win, GC gc, int x, int y, int w, int h, bool sunken);

class PlaylistViewer {
public:
    PlaylistViewer(AppState* state, Display* dpy);
    ~PlaylistViewer();
    
    void show();
    void hide();
    bool isVisible() const { return visible; }
    Window getWindow() const { return win; }
    
    void render();
    void handleInput(int x, int y);
    void handleScroll(int direction); // -1 up, 1 down

private:
    AppState* app;
    Display* dpy;
    Window win;
    GC gc;
    bool visible = false;
    int scrollOffset = 0;
    int maxVisibleItems = 10;
};

struct FileEntry {
    std::string name;
    bool isDir;
};

class FileBrowser {
public:
    FileBrowser(AppState* state, Display* dpy);
    ~FileBrowser();

    void show();
    void hide();
    bool isVisible() const { return visible; }
    Window getWindow() const { return win; }

    void render();
    void handleInput(int x, int y);
    void handleScroll(int direction);

private:
    void refreshList();
    void navigateUp();
    void navigateTo(const std::string& dir);
    void loadFile(const std::string& filename);

    AppState* app;
    Display* dpy;
    Window win;
    GC gc;
    bool visible = false;
    
    std::string currentPath;
    std::vector<FileEntry> entries;
    int scrollOffset = 0;
    int maxVisibleItems = 10;
};

#endif
