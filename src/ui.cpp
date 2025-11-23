#include "ui.h"
#include <iostream>
#include <limits.h>
#include <unistd.h>
#include <iomanip>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

UI::UI(int argc, char** argv) {
    gtk_init(&argc, &argv);
}

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

// --- BUTTON LOGIC (SHUFFLE & REPEAT) ---

void UI::onShuffleClicked(GtkButton* b, gpointer d) {
    UI* ui = (UI*)d;
    ui->playlistMgr->toggleShuffle();
    
    // Visual Feedback
    GtkStyleContext *context = gtk_widget_get_style_context(ui->btnShuffle);
    if (ui->appState.shuffle) {
        gtk_style_context_add_class(context, "active-mode");
    } else {
        gtk_style_context_remove_class(context, "active-mode");
    }
}

void UI::onRepeatClicked(GtkButton* b, gpointer d) {
    UI* ui = (UI*)d;
    ui->playlistMgr->toggleRepeat();
    
    // Visual Feedback: Cycle Labels
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

// --- STYLING (WINAMP 3D EFFECT) ---
void UI::initCSS() {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css = 
        ".tm-window { background-color: " WINAMP_BG_COLOR "; font-size: 12px; }"
        ".tm-window label { color: " WINAMP_FG_COLOR "; font-family: 'Monospace'; font-weight: bold; }"
        
        // BUTTON STYLES
        ".tm-window button { "
        "   background-image: none; "
        "   background-color: " WINAMP_BTN_COLOR "; "
        "   color: #cccccc; "
        "   border: 2px solid; "
        "   border-color: #606060 #202020 #202020 #606060; " // Raised 3D Bevel (Light Top/Left, Dark Bot/Right)
        "   padding: 1px 5px; "
        "   min-height: 20px; "
        "   margin: 1px; "
        "   border-radius: 0px; " // Sharp corners
        "}"
        
        // CLICK EFFECT (Inset)
        ".tm-window button:active { "
        "   background-color: #353535; "
        "   border-color: #202020 #606060 #606060 #202020; " // Inverted Bevel (Dark Top/Left)
        "   color: white; "
        "}"
        
        // ACTIVE MODE (Green Text for Active Shuffle/Repeat)
        ".tm-window button.active-mode { "
        "   color: " WINAMP_FG_COLOR "; "
        "   font-weight: bold; "
        "   background-color: #383838; "
        "}"

        ".tm-window list { background-color: #000000; color: " WINAMP_FG_COLOR "; font-size: 11px; }"
        ".tm-window list row:selected { background-color: #004400; }"
        
        ".tm-window scale trough { min-height: 4px; background-color: #444; border-radius: 2px; }"
        ".tm-window scale highlight { background-color: " WINAMP_FG_COLOR "; border-radius: 2px; }"
        ".tm-window scale slider { min-width: 12px; min-height: 12px; background-color: #silver; border-radius: 50%; }";
        
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

void UI::buildWidgets() {
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "TermuxMusic95");
    gtk_window_set_default_size(GTK_WINDOW(window), 320, 340);
    
    GtkStyleContext *context = gtk_widget_get_style_context(window);
    gtk_style_context_add_class(context, "tm-window");

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(onKeyPress), this);

    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_set_border_width(GTK_CONTAINER(mainBox), 5);
    gtk_container_add(GTK_CONTAINER(window), mainBox);

    drawingArea = gtk_drawing_area_new();
    gtk_widget_set_size_request(drawingArea, -1, 40); 
    gtk_box_pack_start(GTK_BOX(mainBox), drawingArea, FALSE, FALSE, 0);

    lblInfo = gtk_label_new("Ready");
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

    GtkWidget* controlsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); // 0 spacing, padding handles it
    
    GtkWidget* btnPrev = gtk_button_new_with_label("|<");
    GtkWidget* btnPlay = gtk_button_new_with_label("|>");
    GtkWidget* btnPause = gtk_button_new_with_label("||");
    GtkWidget* btnStop = gtk_button_new_with_label("[]");
    GtkWidget* btnNext = gtk_button_new_with_label(">|");
    GtkWidget* btnAdd = gtk_button_new_with_label("+");   
    GtkWidget* btnClear = gtk_button_new_with_label("C"); 
    
    // NEW: Shuffle and Repeat Buttons
    btnShuffle = gtk_button_new_with_label("S"); // S for Shuffle
    btnRepeat = gtk_button_new_with_label("R");  // R for Repeat

    gtk_box_pack_start(GTK_BOX(controlsBox), btnPrev, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnPlay, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnPause, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnStop, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnNext, TRUE, TRUE, 0);
    
    // Group Extras
    gtk_box_pack_start(GTK_BOX(controlsBox), btnShuffle, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnRepeat, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnAdd, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnClear, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(mainBox), controlsBox, FALSE, FALSE, 2);

    GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled, TRUE);
    playlistBox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scrolled), playlistBox);
    gtk_box_pack_start(GTK_BOX(mainBox), scrolled, TRUE, TRUE, 0);

    g_signal_connect(btnPlay, "clicked", G_CALLBACK(onPlayClicked), this);
    g_signal_connect(btnPause, "clicked", G_CALLBACK(onPauseClicked), this);
    g_signal_connect(btnStop, "clicked", G_CALLBACK(onStopClicked), this);
    g_signal_connect(btnAdd, "clicked", G_CALLBACK(onAddClicked), this);
    g_signal_connect(btnClear, "clicked", G_CALLBACK(onClearClicked), this);
    g_signal_connect(btnPrev, "clicked", G_CALLBACK(onPrevClicked), this);
    g_signal_connect(btnNext, "clicked", G_CALLBACK(onNextClicked), this);
    
    // New Signals
    g_signal_connect(btnShuffle, "clicked", G_CALLBACK(onShuffleClicked), this);
    g_signal_connect(btnRepeat, "clicked", G_CALLBACK(onRepeatClicked), this);
    
    g_signal_connect(drawingArea, "draw", G_CALLBACK(Visualizer::onDraw), &appState);
    
    loadLogo();
}

// --- REST OF CALLBACKS ---
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
    gtk_widget_queue_draw(ui->drawingArea);
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
        case GDK_KEY_Return: {
             GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(ui->playlistBox));
             if(row) ui->playlistMgr->onRowActivated(GTK_LIST_BOX(ui->playlistBox), row);
             return TRUE;
        }
    }
    return FALSE;
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
