#include "ui.h"
#include <iostream>
#include <limits.h>
#include <unistd.h>
#include <iomanip>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// --- CONSTANTS ---
const int FULL_WIDTH = 320;
const int FULL_HEIGHT_INIT = 340; 
const int VISUALIZER_FULL_HEIGHT = 40; 
const int MINI_HEIGHT_REPURPOSED = 180; 

UI::UI(int argc, char** argv) {
    gtk_init(&argc, &argv);
}

UI::~UI() {
    // Cleanup refs we explicitly took
    if (playlistBox) g_object_unref(playlistBox);
    if (drawingArea) g_object_unref(drawingArea);
    
    if (playlistMgr) delete playlistMgr;
    if (visualizer) delete visualizer;
    if (player) delete player;
    
    // FIX: Removed the old X11 logoImg cleanup line here
}

// --- HELPERS ---
std::string getAssetPath(const std::string& assetName) {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count != -1) {
        std::string exePath(result, count);
        std::string binDir = exePath.substr(0, exePath.find_last_of("/"));
        return binDir + "/../../assets/icons/" + assetName;
    }
    return "assets/icons/" + assetName;
}

void UI::loadLogo() {
    std::string logoPath = getAssetPath("logo.jpg");
    gtk_window_set_icon_from_file(GTK_WINDOW(window), logoPath.c_str(), NULL);
}

// --- FIXED: MODE LOGIC (Safe Re-parenting) ---
void UI::toggleMiniMode(bool force_resize) {
    is_mini_mode = !is_mini_mode;

    // Get the container where the playlist lives
    GtkWidget* scrolled = gtk_widget_get_parent(playlistBox);
    // If playlist is currently hidden/removed, we need to find the scrolled window differently
    if (!scrolled) {
        // In mini mode, drawingArea is inside scrolled, so we find scrolled via drawingArea
        scrolled = gtk_widget_get_parent(drawingArea);
    }

    if (is_mini_mode) {
        // --- ENTERING MINI MODE ---

        // 1. Remove Visualizer from Top Box
        // (Safe because we hold a g_object_ref from buildWidgets)
        gtk_container_remove(GTK_CONTAINER(visualizerContainerBox), drawingArea);
        gtk_widget_hide(visualizerContainerBox); // Hide the top container

        // 2. Remove Playlist from Scrolled Window
        // (Safe because we hold a g_object_ref)
        gtk_container_remove(GTK_CONTAINER(scrolled), playlistBox);
        
        // 3. Put Visualizer into Scrolled Window (The large area)
        gtk_container_add(GTK_CONTAINER(scrolled), drawingArea);
        gtk_widget_set_size_request(drawingArea, FULL_WIDTH, 120); // Make it big
        gtk_widget_show(drawingArea);

        // 4. Resize Window
        gtk_window_set_default_size(GTK_WINDOW(window), FULL_WIDTH, MINI_HEIGHT_REPURPOSED);
        gtk_window_resize(GTK_WINDOW(window), FULL_WIDTH, MINI_HEIGHT_REPURPOSED);
        
        gtk_button_set_label(GTK_BUTTON(btnMiniMode), "F");
        
    } else {
        // --- ENTERING FULL MODE ---

        // 1. Remove Visualizer from Scrolled Window
        gtk_container_remove(GTK_CONTAINER(scrolled), drawingArea);
        
        // 2. Put Playlist Back into Scrolled Window
        gtk_container_add(GTK_CONTAINER(scrolled), playlistBox);
        gtk_widget_show(playlistBox);
        
        // 3. Put Visualizer Back into Top Box
        gtk_box_pack_start(GTK_BOX(visualizerContainerBox), drawingArea, FALSE, FALSE, 0);
        gtk_box_reorder_child(GTK_BOX(visualizerContainerBox), drawingArea, 0); // Ensure top
        
        // Restore Visualizer Size
        gtk_widget_set_size_request(drawingArea, -1, VISUALIZER_FULL_HEIGHT);
        gtk_widget_show(visualizerContainerBox);
        gtk_widget_show(drawingArea);

        // 4. Restore Window Size
        gtk_window_set_default_size(GTK_WINDOW(window), FULL_WIDTH, FULL_HEIGHT_INIT);
        gtk_window_resize(GTK_WINDOW(window), FULL_WIDTH, FULL_HEIGHT_INIT);
        
        gtk_button_set_label(GTK_BUTTON(btnMiniMode), "M");
    }
}

void UI::onMiniModeClicked(GtkButton* btn, gpointer data) {
    ((UI*)data)->toggleMiniMode();
}

// --- BUTTON LOGIC ---
void UI::onPlayClicked(GtkButton* b, gpointer d) { 
    UI* ui = (UI*)d;
    if (ui->appState.playlist.empty()) return;
    if (ui->appState.current_track_idx == -1) {
        ui->appState.current_track_idx = 0;
        size_t idx = (!ui->appState.play_order.empty()) ? ui->appState.play_order[0] : 0;
        ui->player->load(ui->appState.playlist[idx]);
    }
    ui->player->play(); 
}

void UI::onPauseClicked(GtkButton* b, gpointer d) { ((UI*)d)->player->pause(); }
void UI::onStopClicked(GtkButton* b, gpointer d) { ((UI*)d)->player->stop(); }
void UI::onAddClicked(GtkButton* b, gpointer d) { ((UI*)d)->playlistMgr->addFiles(); }
void UI::onClearClicked(GtkButton* b, gpointer d) { ((UI*)d)->playlistMgr->clear(); }
void UI::onPrevClicked(GtkButton* b, gpointer d) { ((UI*)d)->playlistMgr->playPrev(); }
void UI::onNextClicked(GtkButton* b, gpointer d) { ((UI*)d)->playlistMgr->playNext(); }

void UI::onShuffleClicked(GtkButton* b, gpointer d) {
    UI* ui = (UI*)d;
    ui->playlistMgr->toggleShuffle();
    GtkStyleContext *context = gtk_widget_get_style_context(ui->btnShuffle);
    if (ui->appState.shuffle) gtk_style_context_add_class(context, "active-mode");
    else gtk_style_context_remove_class(context, "active-mode");
}

void UI::onRepeatClicked(GtkButton* b, gpointer d) {
    UI* ui = (UI*)d;
    ui->playlistMgr->toggleRepeat();
    GtkStyleContext *context = gtk_widget_get_style_context(ui->btnRepeat);
    switch(ui->appState.repeatMode) {
        case REP_OFF: 
            gtk_button_set_label(GTK_BUTTON(ui->btnRepeat), "R"); 
            gtk_style_context_remove_class(context, "active-mode");
            break;
        case REP_ALL: 
            gtk_button_set_label(GTK_BUTTON(ui->btnRepeat), "R-A"); 
            gtk_style_context_add_class(context, "active-mode");
            break;
        case REP_ONE: 
            gtk_button_set_label(GTK_BUTTON(ui->btnRepeat), "R-1"); 
            gtk_style_context_add_class(context, "active-mode");
            break;
    }
}

// --- SLIDER & UI UPDATES ---
void UI::onVolumeChanged(GtkRange* range, gpointer data) { ((UI*)data)->player->setVolume(gtk_range_get_value(range) / 100.0); }
gboolean UI::onSeekPress(GtkWidget* w, GdkEvent* e, gpointer d) { ((UI*)d)->isSeeking = true; return FALSE; }
gboolean UI::onSeekRelease(GtkWidget* w, GdkEvent* e, gpointer d) { 
    UI* ui = (UI*)d; ui->isSeeking = false; 
    ui->player->seek(gtk_range_get_value(GTK_RANGE(ui->seekScale))); return FALSE; 
}
void UI::onSeekChanged(GtkRange* range, gpointer data) {
    UI* ui = (UI*)data; 
    if (!ui->isSeeking) ui->player->seek(gtk_range_get_value(range)); 
}

gboolean UI::onUpdateTick(gpointer data) {
    UI* ui = (UI*)data;
    // Always update visualizer if playing
    if (ui->player && ui->appState.playing) {
        gtk_widget_queue_draw(ui->drawingArea);
    
        double current = ui->player->getPosition();
        double duration = ui->player->getDuration();
        if (!ui->isSeeking && duration > 0) {
            g_signal_handlers_block_by_func(ui->seekScale, (void*)onSeekChanged, ui);
            gtk_range_set_range(GTK_RANGE(ui->seekScale), 0, duration);
            gtk_range_set_value(GTK_RANGE(ui->seekScale), current);
            g_signal_handlers_unblock_by_func(ui->seekScale, (void*)onSeekChanged, ui);
        }
        int cM = (int)current / 60; int cS = (int)current % 60;
        int dM = (int)duration / 60; int dS = (int)duration % 60;
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << cM << ":" << std::setw(2) << cS << " / " << std::setw(2) << dM << ":" << std::setw(2) << dS;
        gtk_label_set_text(GTK_LABEL(ui->lblInfo), oss.str().c_str());
    }
    return TRUE;
}

gboolean UI::onKeyPress(GtkWidget* widget, GdkEventKey* event, gpointer data) {
    UI* ui = (UI*)data;
    switch (event->keyval) {
        case GDK_KEY_Up: ui->playlistMgr->selectPrev(); return TRUE;
        case GDK_KEY_Down: ui->playlistMgr->selectNext(); return TRUE;
        case GDK_KEY_Delete: ui->playlistMgr->deleteSelected(); return TRUE;
        case GDK_KEY_space: if(ui->appState.playing) ui->player->pause(); else UI::onPlayClicked(NULL, ui); return TRUE;
        case GDK_KEY_M: ui->toggleMiniMode(); return TRUE; 
        case GDK_KEY_Return: {
             GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(ui->playlistBox));
             if(row) ui->playlistMgr->onRowActivated(GTK_LIST_BOX(ui->playlistBox), row);
             return TRUE;
        }
    }
    return FALSE;
}

// --- STYLING ---
void UI::initCSS() {
    GtkCssProvider *provider = gtk_css_provider_new();
    
    // Determine path to style.css
    std::string cssPath;
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    
    if (count != -1) {
        std::string exePath(result, count);
        std::string binDir = exePath.substr(0, exePath.find_last_of("/"));
        // Binary is in build/bin/, styles are in assets/
        cssPath = binDir + "/../../assets/style.css";
    } else {
        cssPath = "assets/style.css";
    }

    GError *error = NULL;
    gtk_css_provider_load_from_path(provider, cssPath.c_str(), &error);
    
    if (error) {
        std::cerr << "Warning: Failed to load CSS from " << cssPath << "\n" 
                  << "Error: " << error->message << std::endl;
        g_error_free(error);
    }

    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

// --- WIDGET CONSTRUCTION ---
void UI::buildWidgets() {
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "TermuxMusic95");
    
    gtk_window_set_default_size(GTK_WINDOW(window), FULL_WIDTH, FULL_HEIGHT_INIT);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE); 

    GtkStyleContext *context = gtk_widget_get_style_context(window);
    gtk_style_context_add_class(context, "tm-window");

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(onKeyPress), this);

    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_set_border_width(GTK_CONTAINER(mainBox), 5);
    gtk_container_add(GTK_CONTAINER(window), mainBox);

    // 1. VISUALIZER CONTAINER
    visualizerContainerBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(mainBox), visualizerContainerBox, FALSE, FALSE, 0);

    drawingArea = gtk_drawing_area_new();
    g_object_ref(drawingArea); // IMPORTANT: We own this ref now
    gtk_widget_set_size_request(drawingArea, -1, VISUALIZER_FULL_HEIGHT); 
    gtk_box_pack_start(GTK_BOX(visualizerContainerBox), drawingArea, FALSE, FALSE, 0);

    // 2. PERMANENT INFO AREA
    lblInfo = gtk_label_new("Ready");
    gtk_box_pack_start(GTK_BOX(mainBox), lblInfo, FALSE, FALSE, 2);

    seekScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(seekScale), FALSE);
    gtk_box_pack_start(GTK_BOX(mainBox), seekScale, FALSE, FALSE, 2);
    
    g_signal_connect(seekScale, "button-press-event", G_CALLBACK(onSeekPress), this);
    g_signal_connect(seekScale, "button-release-event", G_CALLBACK(onSeekRelease), this);
    g_signal_connect(seekScale, "value-changed", G_CALLBACK(onSeekChanged), this);

    // 3. Volume Bar
    GtkWidget* volBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget* lblVol = gtk_label_new("Vol:");
    volScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(volScale), FALSE);
    gtk_range_set_value(GTK_RANGE(volScale), 100);
    gtk_widget_set_hexpand(volScale, TRUE);
    gtk_box_pack_start(GTK_BOX(volBox), lblVol, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(volBox), volScale, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(mainBox), volBox, FALSE, FALSE, 2);
    g_signal_connect(volScale, "value-changed", G_CALLBACK(onVolumeChanged), this);

    // 4. Controls
    GtkWidget* controlsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    
    GtkWidget* btnPrev = gtk_button_new_with_label("|<");
    GtkWidget* btnPlay = gtk_button_new_with_label("|>");
    GtkWidget* btnPause = gtk_button_new_with_label("||");
    GtkWidget* btnStop = gtk_button_new_with_label("[]");
    GtkWidget* btnNext = gtk_button_new_with_label(">|");
    GtkWidget* btnAdd = gtk_button_new_with_label("+");   
    GtkWidget* btnClear = gtk_button_new_with_label("C"); 
    btnShuffle = gtk_button_new_with_label("S");
    btnRepeat = gtk_button_new_with_label("R");
    btnMiniMode = gtk_button_new_with_label("M"); 

    gtk_box_pack_start(GTK_BOX(controlsBox), btnPrev, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnPlay, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnPause, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnStop, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnNext, TRUE, TRUE, 0);
    
    gtk_box_pack_start(GTK_BOX(controlsBox), btnShuffle, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnRepeat, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnMiniMode, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnAdd, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnClear, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(mainBox), controlsBox, FALSE, FALSE, 2);

    // 5. Playlist
    GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled, TRUE); 
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled), 150); 
    playlistBox = gtk_list_box_new();
    
    // IMPORTANT: Take ownership so we can move it later
    g_object_ref(playlistBox);
    
    gtk_container_add(GTK_CONTAINER(scrolled), playlistBox);
    gtk_box_pack_start(GTK_BOX(mainBox), scrolled, TRUE, TRUE, 0);

    // Signals
    g_signal_connect(btnPlay, "clicked", G_CALLBACK(onPlayClicked), this);
    g_signal_connect(btnPause, "clicked", G_CALLBACK(onPauseClicked), this);
    g_signal_connect(btnStop, "clicked", G_CALLBACK(onStopClicked), this);
    g_signal_connect(btnAdd, "clicked", G_CALLBACK(onAddClicked), this);
    g_signal_connect(btnClear, "clicked", G_CALLBACK(onClearClicked), this);
    g_signal_connect(btnPrev, "clicked", G_CALLBACK(onPrevClicked), this);
    g_signal_connect(btnNext, "clicked", G_CALLBACK(onNextClicked), this);
    g_signal_connect(btnShuffle, "clicked", G_CALLBACK(onShuffleClicked), this);
    g_signal_connect(btnRepeat, "clicked", G_CALLBACK(onRepeatClicked), this);
    g_signal_connect(btnMiniMode, "clicked", G_CALLBACK(onMiniModeClicked), this);
    g_signal_connect(drawingArea, "draw", G_CALLBACK(Visualizer::onDraw), &appState);
    
    loadLogo();
}

int UI::run() {
    initCSS();
    player = new Player(&appState);
    buildWidgets(); 
    playlistMgr = new PlaylistManager(&appState, player, playlistBox);
    visualizer = new Visualizer(&appState);
    player->setEOSCallback([](void* data){ ((PlaylistManager*)data)->autoAdvance(); }, playlistMgr);
    g_signal_connect(playlistBox, "row-activated", G_CALLBACK(+[](GtkListBox* b, GtkListBoxRow* r, gpointer d){
        ((PlaylistManager*)d)->onRowActivated(b, r);
    }), playlistMgr);
    g_timeout_add(100, onUpdateTick, this);
    gtk_widget_show_all(window);
    gtk_main();
    delete playlistMgr; delete visualizer; delete player;
    return 0;
}
