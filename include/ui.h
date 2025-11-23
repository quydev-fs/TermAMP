#ifndef UI_H
#define UI_H

#include "common.h"
#include "player.h"
#include "playlist.h"
#include "visualizer.h"

class UI {
public:
    UI(int argc, char** argv);
    int run();

private:
    void initCSS();
    void buildWidgets();
    void loadLogo();
    
    static void onPlayClicked(GtkButton* btn, gpointer data);
    static void onPauseClicked(GtkButton* btn, gpointer data);
    static void onStopClicked(GtkButton* btn, gpointer data);
    static void onAddClicked(GtkButton* btn, gpointer data);
    static void onClearClicked(GtkButton* btn, gpointer data);
    static void onPrevClicked(GtkButton* btn, gpointer data);
    static void onNextClicked(GtkButton* btn, gpointer data);
    
    // NEW HANDLERS
    static void onShuffleClicked(GtkButton* btn, gpointer data);
    static void onRepeatClicked(GtkButton* btn, gpointer data);
    
    static void onVolumeChanged(GtkRange* range, gpointer data);
    static gboolean onSeekPress(GtkWidget* widget, GdkEvent* event, gpointer data);
    static gboolean onSeekRelease(GtkWidget* widget, GdkEvent* event, gpointer data);
    static void onSeekChanged(GtkRange* range, gpointer data);
    static gboolean onUpdateTick(gpointer data);
    static gboolean onKeyPress(GtkWidget* widget, GdkEventKey* event, gpointer data);

    AppState appState;
    Player* player;
    PlaylistManager* playlistMgr;
    Visualizer* visualizer;

    GtkWidget* window;
    GtkWidget* drawingArea; 
    GtkWidget* playlistBox; 
    GtkWidget* lblInfo;
    GtkWidget* seekScale;
    GtkWidget* volScale;
    
    // NEW BUTTON REFERENCES (To change text/color)
    GtkWidget* btnShuffle;
    GtkWidget* btnRepeat;
    
    bool isSeeking = false;
};

#endif
