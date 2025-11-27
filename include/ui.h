#ifndef UI_H
#define UI_H

#include "common.h"
#include "player.h"
#include "playlist.h"
#include "visualizer.h"
// Include the new Utils
#include "utils.h"

class UI {
public:
    UI(int argc, char** argv);
    ~UI();
    int run();

private:
    // Refactored: initCSS and loadLogo removed (moved to Utils)
    void buildWidgets();
    
    // --- Menu Callbacks ---
    static void onAboutClicked(GtkMenuItem* item, gpointer data);

    // --- Mode Control ---
    void toggleMiniMode(bool force_resize = false); 
    static void onMiniModeClicked(GtkButton* btn, gpointer data);

    // --- Signal Handlers ---
    static void onPlayClicked(GtkButton* btn, gpointer data);
    static void onPauseClicked(GtkButton* btn, gpointer data);
    static void onStopClicked(GtkButton* btn, gpointer data);
    static void onAddClicked(GtkButton* btn, gpointer data);
    static void onClearClicked(GtkButton* btn, gpointer data);
    static void onPrevClicked(GtkButton* btn, gpointer data);
    static void onNextClicked(GtkButton* btn, gpointer data);
    static void onShuffleClicked(GtkButton* btn, gpointer data);
    static void onRepeatClicked(GtkButton* btn, gpointer data);

    static void onVolumeChanged(GtkRange* range, gpointer data);
    static gboolean onSeekPress(GtkWidget* widget, GdkEvent* event, gpointer data);
    static gboolean onSeekRelease(GtkWidget* widget, GdkEvent* event, gpointer data);
    static void onSeekChanged(GtkRange* range, gpointer data);

    // --- Equalizer Handlers ---
    static void onEqToggled(GtkToggleButton* toggle, gpointer data);
    static void onEqBandChanged(GtkRange* range, gpointer data);
    static void onEqPresetChanged(GtkComboBox* combo, gpointer data);
    static void onEqResetClicked(GtkButton* btn, gpointer data);

    // --- Crossfading Handlers ---
    static void onCrossfadeToggled(GtkToggleButton* toggle, gpointer data);
    static void onCrossfadeDurationChanged(GtkRange* range, gpointer data);

    // --- Audio Conversion Handlers ---
    static void onConvertAudioClicked(GtkButton* btn, gpointer data);
    static void onConvertFormatChanged(GtkComboBox* combo, gpointer data);

    static gboolean onUpdateTick(gpointer data);
    static gboolean onKeyPress(GtkWidget* widget, GdkEventKey* event, gpointer data);

    // Helper functions
    void updateAlbumArt();

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

    GtkWidget* visualizerContainerBox;

    // Equalizer widgets
    GtkWidget* eqToggle;
    GtkWidget* eqPresetCombo;
    GtkWidget* eqBandsBox;
    std::vector<GtkWidget*> eqSliders;

    // Crossfading widgets
    GtkWidget* crossfadeToggle;
    GtkWidget* crossfadeDurationScale;
    GtkWidget* crossfadeDurationLabel;

    // Audio conversion widgets
    GtkWidget* convertBtn;
    GtkWidget* convertFormatCombo;
    GtkWidget* convertProgress;

    // Album art widgets
    GtkWidget* albumArtImage;

    bool isSeeking = false;
    bool is_mini_mode = false;
};

#endif
