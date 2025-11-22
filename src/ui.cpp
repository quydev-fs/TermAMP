#include "ui.h"
#include "playlist.h" 
#include <X11/keysym.h>
#include <unistd.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <vector>

// --- IMAGE LOADER IMPLEMENTATION ---
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

UI::UI(AppState* state) : app(state), dpy(nullptr) {}

UI::~UI() {
    if (logoImg) {
        // XDestroyImage frees the data structure, but we must be careful 
        // if we allocated the data buffer manually. 
        // Usually XDestroyImage frees the .data pointer too.
        logoImg->data = NULL; // Prevent double free if we manage data
        XDestroyImage(logoImg);
    }
    if (dpy) {
        XDestroyWindow(dpy, win);
        XCloseDisplay(dpy);
    }
}

bool UI::init() {
    dpy = XOpenDisplay(NULL);
    if (!dpy) return false;
    
    int screen = DefaultScreen(dpy);
    // Create Window
    win = XCreateSimpleWindow(dpy, RootWindow(dpy, screen), 10, 10, W_WIDTH, W_HEIGHT, 0, 0, 0);
    
    XSelectInput(dpy, win, ExposureMask | ButtonPressMask | Button1MotionMask | KeyPressMask);
    XStoreName(dpy, win, "TermuxMusic95");
    
    // Size Hints (Fixed size)
    XSizeHints* hints = XAllocSizeHints();
    hints->flags = PMinSize | PMaxSize;
    hints->min_width = hints->max_width = W_WIDTH;
    hints->min_height = hints->max_height = W_HEIGHT;
    XSetWMNormalHints(dpy, win, hints);
    XFree(hints);
    
    // Close Handler
    wmDeleteMessage = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, win, &wmDeleteMessage, 1);
    
    XMapWindow(dpy, win);
    gc = XCreateGC(dpy, win, 0, NULL);

    // --- LOAD LOGO & SET ICON ---
    loadLogo();

    return true;
}

void UI::loadLogo() {
    int w, h, channels;
    unsigned char* data = stbi_load("assets/logo.jpg", &w, &h, &channels, 4); // Force 4 channels (RGBA)
    
    if (!data) {
        std::cerr << "Warning: Could not load assets/logo.jpg" << std::endl;
        return;
    }

    logoW = w;
    logoH = h;

    // 1. PREPARE FOR TITLE BAR (XImage)
    // X11 usually expects BGRA (Little Endian) or ARGB (Big Endian). 
    // Most Android/Termux X11 servers are Little Endian (BGRA).
    // stbi gives us R G B A. We need to swap R and B.
    
    // We allocate a buffer for the XImage
    char* xImageData = (char*)malloc(w * h * 4);
    
    // We also prepare a buffer for the _NET_WM_ICON (Needs ARGB unsigned long)
    // Format: [width, height, p1, p2, p3...]
    std::vector<unsigned long> iconData;
    iconData.push_back(w);
    iconData.push_back(h);
    
    for (int i = 0; i < w * h; i++) {
        unsigned char r = data[i*4 + 0];
        unsigned char g = data[i*4 + 1];
        unsigned char b = data[i*4 + 2];
        unsigned char a = data[i*4 + 3];

        // XImage Buffer (BGRA for display)
        xImageData[i*4 + 0] = b;
        xImageData[i*4 + 1] = g;
        xImageData[i*4 + 2] = r;
        xImageData[i*4 + 3] = a; // Alpha usually ignored in SimpleWindow but good to have

        // Icon Buffer (ARGB for Dock)
        // _NET_WM_ICON expects 32-bit integers in ARGB format
        unsigned long argb = ((unsigned long)a << 24) | ((unsigned long)r << 16) | ((unsigned long)g << 8) | b;
        iconData.push_back(argb);
    }

    // Create XImage for drawing inside the app
    logoImg = XCreateImage(dpy, DefaultVisual(dpy, 0), 24, ZPixmap, 0, xImageData, w, h, 32, 0);

    // 2. SET DOCK/TASKBAR ICON (_NET_WM_ICON)
    Atom netWmIcon = XInternAtom(dpy, "_NET_WM_ICON", False);
    Atom cardinal = XInternAtom(dpy, "CARDINAL", False);
    
    XChangeProperty(dpy, win, netWmIcon, cardinal, 32, PropModeReplace, 
                    (unsigned char*)iconData.data(), iconData.size());

    stbi_image_free(data);
}

void UI::drawBevel(int x, int y, int w, int h, bool sunken) {
    XSetForeground(dpy, gc, sunken ? C_BTN_D : C_BTN_L);
    XDrawLine(dpy, win, gc, x, y, x+w-2, y);
    XDrawLine(dpy, win, gc, x, y, x, y+h-2);
    XSetForeground(dpy, gc, sunken ? C_BTN_L : C_BTN_D);
    XDrawLine(dpy, win, gc, x+w-1, y, x+w-1, y+h-1);
    XDrawLine(dpy, win, gc, x, y+h-1, x+w-1, y+h-1);
}

void UI::drawButton(int x, int y, int w, int h, const char* label, bool pressed) {
    XSetForeground(dpy, gc, C_FACE);
    XFillRectangle(dpy, win, gc, x+1, y+1, w-2, h-2);
    drawBevel(x, y, w, h, pressed);
    XSetForeground(dpy, gc, pressed ? C_TXT_GRN : 0xC0C0C0);
    drawText(x + (w - (strlen(label)*6))/2, y + (h+4)/2, label, 0);
}

void UI::drawText(int x, int y, const char* str, unsigned long color) {
    if(color != 0) XSetForeground(dpy, gc, color);
    XDrawString(dpy, win, gc, x, y, str, strlen(str));
}

void UI::render() {
    // Main Background
    XSetForeground(dpy, gc, C_FACE);
    XFillRectangle(dpy, win, gc, 0, 0, W_WIDTH, W_HEIGHT);
    
    // --- TITLE BAR DRAWING ---
    XSetForeground(dpy, gc, C_TITLE_BG);
    XFillRectangle(dpy, win, gc, 0, 0, W_WIDTH, 14);
    
    // DRAW LOGO ON TITLE BAR
    if (logoImg) {
        // Draw logo at (2, 2) with size 10x10 (scaled or clipped? XPutImage clips)
        // Ideally your logo.jpg should be small (e.g., 16x16 or 32x32).
        // If it's huge, it will cover the app. Let's assume it is resized or we clip it.
        // Winamp logo is usually top left.
        
        // We copy the image to the window. 
        // Source X,Y = 0,0. Dest X,Y = 2,1. Width/Height = min(logoW, 12) to fit bar.
        int drawW = std::min(logoW, 12);
        int drawH = std::min(logoH, 12);
        XPutImage(dpy, win, gc, logoImg, 0, 0, 3, 1, drawW, drawH);
    }

    drawBevel(0, 0, W_WIDTH, 14, false);
    
    // Title Text (Shifted slightly right to make room for logo)
    static int scroll_x = 0;
    static int scroll_tick = 0;
    std::string disp = app->current_title + " *** " + app->current_title;
    if (++scroll_tick % 5 == 0) scroll_x++;
    if (scroll_x > (int)app->current_title.length() * 7) scroll_x = 0;
    std::string sub = disp.substr(scroll_x / 7, 30);
    drawText(20, 11, sub.c_str(), 0xFFFFFF); // X changed from 10 to 20
    
    // --- REST OF UI (Unchanged) ---
    XSetForeground(dpy, gc, C_VIS_BG);
    XFillRectangle(dpy, win, gc, 20, 24, 76, 16);
    drawBevel(19, 23, 78, 18, true);
    
    for(int i=0; i<16; i++) {
        int h = app->viz_bands[i];
        int x = 21 + (i * 5);
        if(h > 0) {
            XSetForeground(dpy, gc, C_TXT_GRN);
            int gh = std::min(h, 12);
            XFillRectangle(dpy, win, gc, x, 40-gh, 4, gh);
            if (h > 12) {
               XSetForeground(dpy, gc, C_TXT_YEL);
               XFillRectangle(dpy, win, gc, x, 40-h, 4, h-12);
            }
        }
    }

    XSetForeground(dpy, gc, C_VIS_BG);
    XFillRectangle(dpy, win, gc, 35, 45, 46, 20);
    drawBevel(34, 44, 48, 22, true);
    long secs = (app->sample_rate > 0) ? app->current_frame / app->sample_rate : 0;
    char ts[16]; sprintf(ts, "%02ld:%02ld", secs/60, secs%60);
    drawText(42, 60, ts, C_TXT_GRN);

    char meta[32]; sprintf(meta, "%d", (int)app->bitrate);
    drawText(112, 42, meta, C_TXT_GRN); drawText(155, 42, "kbps", C_TXT_GRN);
    sprintf(meta, "%d", (int)app->sample_rate/1000);
    drawText(112, 52, meta, C_TXT_GRN); drawText(155, 52, "kHz", C_TXT_GRN);

    int sw = 250; int sx = 12; int sy = 72;
    drawBevel(sx, sy, sw, 10, true);
    if (app->total_frames > 0) {
        double pct = (double)app->current_frame / (double)app->total_frames;
        int nx = sx + (int)(pct * (sw - 10));
        XSetForeground(dpy, gc, C_BTN_L);
        XFillRectangle(dpy, win, gc, nx, sy+1, 10, 8);
        drawBevel(nx, sy+1, 10, 8, false);
    }

    drawBevel(100, 90, 100, 5, true);
    int vx = 100 + (int)((app->volume/100.0) * 90);
    XSetForeground(dpy, gc, C_BTN_L);
    XFillRectangle(dpy, win, gc, vx, 87, 10, 10);
    drawBevel(vx, 87, 10, 10, false);

    int by = 88;
    drawButton(16, by, 20, 18, "|<", false);
    drawButton(39, by, 20, 18, ">", app->playing && !app->paused);
    drawButton(62, by, 20, 18, "||", app->paused);
    drawButton(85, by, 20, 18, "[]", false);
    drawButton(108, by, 20, 18, ">|", false);

    // Shuffle/Repeat/Save
    drawButton(200, 90, 20, 12, "SH", app->shuffle);
    const char* rpLabel = (app->repeatMode == REP_ONE) ? "1" : (app->repeatMode == REP_ALL ? "AL" : "RP");
    drawButton(225, 90, 20, 12, rpLabel, app->repeatMode != REP_OFF);
    drawButton(250, 90, 20, 12, "SV", false); 
}

void UI::handleInput(int x, int y) {
    if (y >= 88 && y <= 106) {
        if (x>=16 && x<36) { if(app->track_idx > 0) app->track_idx--; app->playing=true; }
        else if (x>=39 && x<59) { app->playing = true; app->paused = false; }
        else if (x>=62 && x<82) { if(app->playing) app->paused = !app->paused; }
        else if (x>=85 && x<105) { app->playing = false; app->paused = false; app->current_frame=0; }
        else if (x>=108 && x<128) { if(app->track_idx < app->playlist.size()-1) app->track_idx++; app->playing=true; }
    }
    
    if (y >= 90 && y <= 102) {
        if (x >= 200 && x <= 220) {
            app->shuffle = !app->shuffle;
            toggleShuffle(*app);
        }
        else if (x >= 225 && x <= 245) {
            int mode = app->repeatMode;
            mode = (mode + 1) % 3;
            app->repeatMode = mode;
        }
        else if (x >= 250 && x <= 270) {
            savePlaylist(*app);
        }
    }

    if (y >= 72 && y <= 82 && x >= 12 && x <= 262) {
        app->seek_pos = (double)(x - 12) / 250.0;
        app->seek_request = true;
    }
    if (y >= 85 && y <= 100 && x >= 100 && x <= 200) {
        int v = x - 100;
        if (v < 0) v = 0; if (v > 100) v = 100;
        app->volume = v;
    }
}

void UI::handleKey(KeySym ks) {
    if (ks == XK_x) { app->playing = true; app->paused = false; }
    if (ks == XK_c) { if(app->playing) app->paused = !app->paused; }
    if (ks == XK_v) { app->playing = false; }
    if (ks == XK_z) { if(app->track_idx > 0) app->track_idx--; app->playing=true; }
    if (ks == XK_b) { if(app->track_idx < app->playlist.size()-1) app->track_idx++; app->playing=true; }
    if (ks == XK_s) { savePlaylist(*app); }
    if (ks == XK_r) { 
        int mode = app->repeatMode;
        mode = (mode + 1) % 3;
        app->repeatMode = mode;
    } 
}

void UI::runLoop() {
    XEvent e;
    while (app->running) {
        if (XPending(dpy)) {
            XNextEvent(dpy, &e);
            if (e.type == ClientMessage && (unsigned long)e.xclient.data.l[0] == wmDeleteMessage) app->running = false;
            else if (e.type == Expose) render();
            else if (e.type == ButtonPress) { handleInput(e.xbutton.x, e.xbutton.y); render(); }
            else if (e.type == MotionNotify && (e.xmotion.state & Button1Mask)) { handleInput(e.xmotion.x, e.xmotion.y); render(); }
            else if (e.type == KeyPress) { handleKey(XLookupKeysym(&e.xkey, 0)); render(); }
        } else {
            render();
            usleep(33000);
        }
    }
}
