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
    
    // --- Mode Control ---
    void toggleMiniMode(bool force_resize = false); 
    static void onMiniModeClicked(GtkButton* btn, gpointer data);

    // --- Signal Handlers (ALL DECLARED HERE) ---
    static void onPlayClicked(GtkButton* btn, gpointer data);
    static void onPauseClicked(GtkButton* btn, gpointer data);
    static void onStopClicked(GtkButton* btn, gpointer data);
    static void onAddClicked(GtkButton* btn, gpointer data);
    static void onClearClicked(GtkButton* btn, gpointer data);
    static void onPrevClicked(GtkButton* btn, gpointer data);
    static void onNextClicked(GtkButton* btn, gpointer data);
    static void onShuffleClicked(GtkButton* btn, gpointer data);
    static void onRepeatClicked(GtkButton* btn, gpointer data);
    
    // --- Slider Signals ---
    static void onVolumeChanged(GtkRange* range, gpointer data);
    static gboolean onSeekPress(GtkWidget* widget, GdkEvent* event, gpointer data);
    static gboolean onSeekRelease(GtkWidget* widget, GdkEvent* event, gpointer data);
    static void onSeekChanged(GtkRange* range, gpointer data);
    
    // --- Update Loops ---
    static gboolean onUpdateTick(gpointer data);
    static gboolean onKeyPress(GtkWidget* widget, GdkEventKey* event, gpointer data);

    // --- Members ---
    AppState appState;
    Player* player;
    PlaylistManager* playlistMgr;
    Visualizer* visualizer;

    // Widgets
    GtkWidget* window;
    GtkWidget* drawingArea; 
    GtkWidget* playlistBox; 
    GtkWidget* lblInfo;
    GtkWidget* seekScale;
    GtkWidget* volScale;
    GtkWidget* btnShuffle;
    GtkWidget* btnRepeat;
    GtkWidget* btnMiniMode;
    
    // Containers
    GtkWidget* visualizerContainerBox;

    // State
    bool isSeeking = false;
    bool is_mini_mode = false;
};

#endif
