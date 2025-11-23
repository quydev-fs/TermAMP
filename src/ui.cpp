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
const int MINI_HEIGHT_REPURPOSED = 180; // Height for controls + big visualizer

UI::UI(int argc, char** argv) {
    gtk_init(&argc, &argv);
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

    // 1. Locate the Scrolled Window (Playlist Container)
    // Since playlistBox might be removed, we find the ScrolledWindow relative to the main layout
    // It is always the last child of the mainBox.
    GtkWidget* mainBox = gtk_widget_get_parent(visualizerContainerBox);
    GList* children = gtk_container_get_children(GTK_CONTAINER(mainBox));
    GtkWidget* scrolled = GTK_WIDGET(g_list_last(children)->data);
    g_list_free(children);

    if (is_mini_mode) {
        // --- SWITCHING TO MINI MODE ---

        // A. Move Drawing Area: Box -> Scrolled Window
        g_object_ref(drawingArea); // Protect from destruction
        gtk_container_remove(GTK_CONTAINER(visualizerContainerBox), drawingArea); // Remove from top
        
        // B. Swap Playlist out
        g_object_ref(playlistBox); // Protect from destruction
        gtk_container_remove(GTK_CONTAINER(scrolled), playlistBox); // Remove list
        
        // C. Put Drawing Area into Scrolled Window
        gtk_container_add(GTK_CONTAINER(scrolled), drawingArea);
        
        // Resize Visualizer to fill the new space
        gtk_widget_set_size_request(drawingArea, FULL_WIDTH, 120); 
        
        // D. Resize Window
        gtk_window_set_default_size(GTK_WINDOW(window), FULL_WIDTH, MINI_HEIGHT_REPURPOSED);
        gtk_window_resize(GTK_WINDOW(window), FULL_WIDTH, MINI_HEIGHT_REPURPOSED);
        
        gtk_button_set_label(GTK_BUTTON(btnMiniMode), "F");
        
        // Release our temporary references (container now owns them)
        g_object_unref(drawingArea);
        g_object_unref(playlistBox);

    } else {
        // --- SWITCHING TO FULL MODE ---

        // A. Move Drawing Area: Scrolled Window -> Box
        g_object_ref(drawingArea); 
        gtk_container_remove(GTK_CONTAINER(scrolled), drawingArea);
        
        // B. Put Playlist back
        g_object_ref(playlistBox);
        gtk_container_add(GTK_CONTAINER(scrolled), playlistBox);
        
        // C. Put Drawing Area back to top
        gtk_box_pack_start(GTK_BOX(visualizerContainerBox), drawingArea, FALSE, FALSE, 0);
        gtk_box_reorder_child(GTK_BOX(visualizerContainerBox), drawingArea, 0); // Ensure it's at the top
        
        // Restore Size
        gtk_widget_set_size_request(drawingArea, -1, VISUALIZER_FULL_HEIGHT);
        
        // D. Resize Window
        gtk_window_set_default_size(GTK_WINDOW(window), FULL_WIDTH, FULL_HEIGHT_INIT);
        gtk_window_resize(GTK_WINDOW(window), FULL_WIDTH, FULL_HEIGHT_INIT);
        
        gtk_button_set_label(GTK_BUTTON(btnMiniMode), "M");

        // Release refs
        g_object_unref(drawingArea);
        g_object_unref(playlistBox);
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
    if (ui->player && ui->appState.playing) {
        gtk_widget_queue_draw(ui->drawingArea);
    }
    
    if (ui->player && ui->appState.playing) {
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
    const char *css = 
        ".tm-window { background-color: " WINAMP_BG_COLOR "; font-size: 12px; }"
        ".tm-window label { color: " WINAMP_FG_COLOR "; font-family: 'Monospace'; font-weight: bold; }"
        ".tm-window button { "
        "   background-image: none; background-color: " WINAMP_BTN_COLOR "; "
        "   color: #cccccc; border: 2px solid; "
        "   border-color: #606060 #202020 #202020 #606060; "
        "   padding: 1px 5px; min-height: 20px; margin: 1px; border-radius: 0px; }"
        ".tm-window button:active { "
        "   background-color: #353535; border-color: #202020 #606060 #606060 #202020; color: white; }"
        ".tm-window button.active-mode { "
        "   color: " WINAMP_FG_COLOR "; font-weight: bold; background-color: #383838; }"
        ".tm-window list { background-color: #000000; color: " WINAMP_FG_COLOR "; font-size: 11px; }"
        ".tm-window list row:selected { background-color: #004400; }"
        ".tm-window scale trough { min-height: 4px; background-color: #444; border-radius: 2px; }"
        ".tm-window scale highlight { background-color: " WINAMP_FG_COLOR "; border-radius: 2px; }"
        ".tm-window scale slider { min-width: 12px; min-height: 12px; background-color: #silver; border-radius: 50%; }";
        
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
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
