#ifndef UI_H
#define UI_H

#include "common.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h> // Needed for _NET_WM_ICON

class UI {
public:
    UI(AppState* state);
    ~UI();
    bool init();
    void runLoop();

private:
    void render();
    void handleInput(int x, int y);
    void handleKey(KeySym ks);
    
    // New function to handle logo loading
    void loadLogo();

    void drawBevel(int x, int y, int w, int h, bool sunken);
    void drawButton(int x, int y, int w, int h, const char* label, bool pressed);
    void drawText(int x, int y, const char* str, unsigned long color);

    AppState* app;
    Display* dpy;
    Window win;
    GC gc;
    Atom wmDeleteMessage;
    
    // Logo Image Data
    XImage* logoImg = nullptr; 
    int logoW = 0;
    int logoH = 0;
};

#endif
