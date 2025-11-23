#include "ui.h"
#include <iostream>
#include <limits.h>
#include <unistd.h>

UI::UI(int argc, char** argv) {
    gtk_init(&argc, &argv);
}

// --- HELPERS ---
std::string getAssetPath(const std::string& assetName) {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    std::string fullPath;
    if (count != -1) {
        std::string exePath(result, count);
        size_t lastSlash = exePath.find_last_of("/");
        std::string binDir = exePath.substr(0, lastSlash);
        // Assuming binary is in build/bin, go up two levels to root
        fullPath = binDir + "/../../assets/icons/" + assetName;
    } else {
        fullPath = "assets/icons/" + assetName;
    }
    return fullPath;
}

void UI::initCSS() {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css = 
        ".tm-window { background-color: " WINAMP_BG_COLOR "; font-size: 12px; }"
        ".tm-window label { color: " WINAMP_FG_COLOR "; font-family: 'Monospace'; font-weight: bold; }"
        ".tm-window button { "
        "   background-image: none; background-color: " WINAMP_BTN_COLOR "; "
        "   color: white; border: 1px solid #000; padding: 2px 6px; min-height: 20px; margin: 0px; }"
        ".tm-window button:hover { background-color: #555555; }"
        ".tm-window list { background-color: #000000; color: " WINAMP_FG_COLOR "; font-size: 11px; }"
        ".tm-window list row:selected { background-color: #004400; }";
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

void UI::buildWidgets() {
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "TermuxMusic95");
    gtk_window_set_default_size(GTK_WINDOW(window), 320, 280);
    
    // --- FIX: LOAD LOGO ---
    std::string logoPath = getAssetPath("logo.jpg");
    GError *err = NULL;
    if (!gtk_window_set_icon_from_file(GTK_WINDOW(window), logoPath.c_str(), &err)) {
        // Fallback try
        if (!gtk_window_set_icon_from_file(GTK_WINDOW(window), "assets/icons/logo.jpg", &err)) {
            // std::cerr << "Failed to load icon: " << err->message << std::endl; // Debug
        }
    }

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

    GtkWidget* controlsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    GtkWidget* btnPrev = gtk_button_new_with_label("|<");
    GtkWidget* btnPlay = gtk_button_new_with_label("|>");
    GtkWidget* btnPause = gtk_button_new_with_label("||");
    GtkWidget* btnStop = gtk_button_new_with_label("[]");
    GtkWidget* btnNext = gtk_button_new_with_label(">|");
    GtkWidget* btnAdd = gtk_button_new_with_label("+");
    GtkWidget* btnClear = gtk_button_new_with_label("C");

    gtk_box_pack_start(GTK_BOX(controlsBox), btnPrev, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnPlay, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnPause, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnStop, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controlsBox), btnNext, TRUE, TRUE, 0);
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
    g_signal_connect(drawingArea, "draw", G_CALLBACK(Visualizer::onDraw), &appState);
}

// --- CALLBACKS ---
void UI::onPlayClicked(GtkButton* b, gpointer d) { 
    UI* ui = (UI*)d;
    if (ui->appState.playlist.empty()) return;
    if (ui->appState.current_track_idx == -1) {
        ui->appState.current_track_idx = 0;
        ui->player->load(ui->appState.playlist[ui->appState.play_order[0]]);
    }
    ui->player->play(); 
}
void UI::onPauseClicked(GtkButton* b, gpointer d) { ((UI*)d)->player->pause(); }
void UI::onStopClicked(GtkButton* b, gpointer d) { ((UI*)d)->player->stop(); }
void UI::onAddClicked(GtkButton* b, gpointer d) { ((UI*)d)->playlistMgr->addFiles(); }
void UI::onClearClicked(GtkButton* b, gpointer d) { ((UI*)d)->playlistMgr->clear(); }

// FIX: Connect these to the playNext/playPrev logic
void UI::onPrevClicked(GtkButton* b, gpointer d) { ((UI*)d)->playlistMgr->playPrev(); }
void UI::onNextClicked(GtkButton* b, gpointer d) { ((UI*)d)->playlistMgr->playNext(); }

gboolean UI::onKeyPress(GtkWidget* widget, GdkEventKey* event, gpointer data) {
    UI* ui = (UI*)data;
    switch (event->keyval) {
        case GDK_KEY_Up: ui->playlistMgr->selectPrev(); return TRUE;
        case GDK_KEY_Down: ui->playlistMgr->selectNext(); return TRUE;
        case GDK_KEY_Delete: ui->playlistMgr->deleteSelected(); return TRUE;
        case GDK_KEY_space: 
            if(ui->appState.playing) ui->player->pause(); else ui->player->play(); 
            return TRUE;
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
    g_signal_connect(playlistBox, "row-activated", G_CALLBACK(+[](GtkListBox* b, GtkListBoxRow* r, gpointer d){
        ((PlaylistManager*)d)->onRowActivated(b, r);
    }), playlistMgr);
    g_timeout_add(50, Visualizer::onTick, drawingArea);
    gtk_widget_show_all(window);
    gtk_main();
    delete playlistMgr; delete visualizer; delete player;
    return 0;
}
