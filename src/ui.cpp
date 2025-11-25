#include "ui.h"
#include <iostream>
#include <limits.h>
#include <unistd.h>
#include <iomanip>
#include <sstream>
#include <cstdlib> // For getenv

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// --- CONSTANTS ---
const int FULL_WIDTH = 320;
const int FULL_HEIGHT_INIT = 340; 
const int VISUALIZER_FULL_HEIGHT = 40; 
const int MINI_HEIGHT_REPURPOSED = 180; 

// --- HELPER FUNCTION (UPDATED FOR /etc/TermAMP) ---
std::string getResourcePath(const std::string& assetName) {
    // 1. Define the Configuration Directory
    std::string configDir;
    
    // Try to get Termux PREFIX from environment
    const char* env_prefix = std::getenv("PREFIX");
    if (env_prefix) {
        configDir = std::string(env_prefix) + "/etc/TermAMP/";
    } else {
        // Fallback for standard Termux structure if env is missing
        configDir = "/data/data/com.termux/files/usr/etc/TermAMP/";
    }

    // 2. Strip "assets/" from the requested name
    // The code calls getResourcePath("assets/style.css"), but we want
    // to load it from ".../etc/TermAMP/style.css"
    std::string cleanName = assetName;
    std::string removePrefix = "assets/";
    
    if (cleanName.rfind(removePrefix, 0) == 0) { // starts_with check
        cleanName = cleanName.substr(removePrefix.length());
    }

    return configDir + cleanName;
}

// --- CONSTRUCTOR ---
UI::UI(int argc, char** argv) {
    gtk_init(&argc, &argv);
    player = nullptr;
    playlistMgr = nullptr;
    visualizer = nullptr;
    playlistBox = nullptr;
    drawingArea = nullptr;
}

// --- DESTRUCTOR ---
UI::~UI() {
    if (playlistBox) g_object_unref(playlistBox);
    if (drawingArea) g_object_unref(drawingArea);
    if (playlistMgr) delete playlistMgr;
    if (visualizer) delete visualizer;
    if (player) delete player;
}

// --- LOADING LOGIC ---
void UI::loadLogo() {
    std::string logoPath = getResourcePath("assets/icons/logo.jpg");
    GError *err = NULL;
    if(!gtk_window_set_icon_from_file(GTK_WINDOW(window), logoPath.c_str(), &err)) {
        // Fail silently or log to console, don't crash
        if(err) g_error_free(err);
    }
}

void UI::initCSS() {
    GtkCssProvider *provider = gtk_css_provider_new();
    
    std::string cssPath = getResourcePath("assets/style.css");
    
    GError *error = NULL;
    gtk_css_provider_load_from_path(provider, cssPath.c_str(), &error);
    
    if (error) {
        std::cerr << "CSS Load Error: " << error->message << "\nAttempted Path: " << cssPath << std::endl;
        g_error_free(error);
    } else {
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(), 
            GTK_STYLE_PROVIDER(provider), 
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }
    g_object_unref(provider);
}

// --- MINI MODE LOGIC ---
void UI::toggleMiniMode(bool force_resize) {
    is_mini_mode = !is_mini_mode;

    GtkWidget* scrolled = gtk_widget_get_parent(playlistBox);
    if (!scrolled) {
        scrolled = gtk_widget_get_parent(drawingArea);
    }

    if (is_mini_mode) {
        // --- SWITCH TO MINI MODE ---
        g_object_ref(drawingArea);
        g_object_ref(playlistBox);

        gtk_container_remove(GTK_CONTAINER(visualizerContainerBox), drawingArea);
        gtk_widget_hide(visualizerContainerBox); 

        gtk_container_remove(GTK_CONTAINER(scrolled), playlistBox);
        
        gtk_container_add(GTK_CONTAINER(scrolled), drawingArea);
        gtk_widget_set_size_request(drawingArea, FULL_WIDTH, 120); 
        gtk_widget_show(drawingArea);

        gtk_window_set_default_size(GTK_WINDOW(window), FULL_WIDTH, MINI_HEIGHT_REPURPOSED);
        gtk_window_resize(GTK_WINDOW(window), FULL_WIDTH, MINI_HEIGHT_REPURPOSED);
        
        gtk_button_set_label(GTK_BUTTON(btnMiniMode), "F");
        
        g_object_unref(drawingArea);
        g_object_unref(playlistBox);

    } else {
        // --- SWITCH TO FULL MODE ---
        g_object_ref(drawingArea);
        g_object_ref(playlistBox);

        gtk_container_remove(GTK_CONTAINER(scrolled), drawingArea);
        
        gtk_container_add(GTK_CONTAINER(scrolled), playlistBox);
        gtk_widget_show(playlistBox);
        
        gtk_box_pack_start(GTK_BOX(visualizerContainerBox), drawingArea, FALSE, FALSE, 0);
        gtk_box_reorder_child(GTK_BOX(visualizerContainerBox), drawingArea, 0);
        
        gtk_widget_set_size_request(drawingArea, -1, VISUALIZER_FULL_HEIGHT);
        gtk_widget_show(visualizerContainerBox);
        gtk_widget_show(drawingArea);

        gtk_window_set_default_size(GTK_WINDOW(window), FULL_WIDTH, FULL_HEIGHT_INIT);
        gtk_window_resize(GTK_WINDOW(window), FULL_WIDTH, FULL_HEIGHT_INIT);
        
        gtk_button_set_label(GTK_BUTTON(btnMiniMode), "M");

        g_object_unref(drawingArea);
        g_object_unref(playlistBox);
    }
}

void UI::onMiniModeClicked(GtkButton* btn, gpointer data) {
    ((UI*)data)->toggleMiniMode();
}

// --- BUTTON CALLBACKS ---
void UI::onPlayClicked(GtkButton* b, gpointer d) { 
    UI* ui = (UI*)d;
    if (ui->appState.playlist.empty()) return;
    
    if (ui->appState.paused) {
        ui->player->play();
        return;
    }
    if (ui->appState.playing) return;

    if (ui->appState.current_track_idx == -1) {
        ui->appState.current_track_idx = 0;
        size_t idx = (!ui->appState.play_order.empty()) ? ui->appState.play_order[0] : 0;
        ui->player->load(ui->appState.playlist[idx]);
    } else {
        size_t idx = ui->appState.play_order[ui->appState.current_track_idx];
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

// --- SLIDER UPDATES ---
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
    if (!ui->player) return TRUE;

    if (ui->appState.playing) {
        if (ui->window && gtk_widget_get_visible(ui->window)) {
            gtk_widget_queue_draw(ui->drawingArea);
        }
    
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
        oss << ui->appState.current_track_name << " (" 
            << std::setfill('0') << std::setw(2) << cM << ":" << std::setw(2) << cS 
            << " / " 
            << std::setw(2) << dM << ":" << std::setw(2) << dS << ")";
        gtk_label_set_text(GTK_LABEL(ui->lblInfo), oss.str().c_str());
    } else if (!ui->appState.playlist.empty() && ui->appState.current_track_idx != -1) {
        gtk_label_set_text(GTK_LABEL(ui->lblInfo), ui->appState.current_track_name.c_str());
    } else {
        gtk_label_set_text(GTK_LABEL(ui->lblInfo), "Ready");
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

    visualizerContainerBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(mainBox), visualizerContainerBox, FALSE, FALSE, 0);

    drawingArea = gtk_drawing_area_new();
    g_object_ref(drawingArea); 
    gtk_widget_set_size_request(drawingArea, -1, VISUALIZER_FULL_HEIGHT); 
    gtk_box_pack_start(GTK_BOX(visualizerContainerBox), drawingArea, FALSE, FALSE, 0);

    lblInfo = gtk_label_new("Ready");
    gtk_label_set_ellipsize(GTK_LABEL(lblInfo), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start(GTK_BOX(mainBox), lblInfo, FALSE, FALSE, 2);

    seekScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(seekScale), FALSE);
    gtk_box_pack_start(GTK_BOX(mainBox), seekScale, FALSE, FALSE, 2);
    
    g_signal_connect(seekScale, "button-press-event", G_CALLBACK(onSeekPress), this);
    g_signal_connect(seekScale, "button-release-event", G_CALLBACK(onSeekRelease), this);
    g_signal_connect(seekScale, "value-changed", G_CALLBACK(onSeekChanged), this);

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

    GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled, TRUE); 
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled), 150); 
    playlistBox = gtk_list_box_new();
    g_object_ref(playlistBox); 
    gtk_container_add(GTK_CONTAINER(scrolled), playlistBox);
    gtk_box_pack_start(GTK_BOX(mainBox), scrolled, TRUE, TRUE, 0);

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
    
    // NOTE: Visualizer object initialized in run(), pass 'visualizer' here (it's a member of UI class)
    // But wait, buildWidgets is called inside run(). 
    // We need to make sure visualizer is initialized BEFORE buildWidgets is called.
    // My previous UI::run logic ensures this.
    g_signal_connect(drawingArea, "draw", G_CALLBACK(Visualizer::onDraw), visualizer);
    
    loadLogo();
}

int UI::run() {
    initCSS();
    player = new Player(&appState);
    visualizer = new Visualizer(&appState); // Init here!
    buildWidgets(); 
    playlistMgr = new PlaylistManager(&appState, player, playlistBox);
    player->setEOSCallback([](void* data){ ((PlaylistManager*)data)->autoAdvance(); }, playlistMgr);
    g_signal_connect(playlistBox, "row-activated", G_CALLBACK(+[](GtkListBox* b, GtkListBoxRow* r, gpointer d){
        ((PlaylistManager*)d)->onRowActivated(b, r);
    }), playlistMgr);
    g_timeout_add(100, onUpdateTick, this);
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
