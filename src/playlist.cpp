#include "playlist.h"
#include <iostream>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <random>
#include <chrono>

PlaylistManager::PlaylistManager(AppState* state, Player* pl, GtkWidget* list) 
    : app(state), player(pl), listBox(list) {
    parentWindow = gtk_widget_get_toplevel(listBox);
}

// --- HELPERS ---
void PlaylistManager::highlightCurrentTrack() {
    if (app->current_track_idx < 0 || app->current_track_idx >= (int)app->play_order.size()) return;
    int actual_playlist_index = app->play_order[app->current_track_idx];
    GtkListBoxRow* row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(listBox), actual_playlist_index);
    if (row) gtk_list_box_select_row(GTK_LIST_BOX(listBox), row);
}

// --- STATE TOGGLES (NEW) ---
void PlaylistManager::toggleShuffle() {
    app->shuffle = !app->shuffle;
    
    // If turning ON, shuffle the vector
    if (app->shuffle) {
        size_t current_real_idx = 0;
        if (app->current_track_idx != -1) {
            current_real_idx = app->play_order[app->current_track_idx];
        }

        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::shuffle(app->play_order.begin(), app->play_order.end(), std::default_random_engine(seed));

        // Find where the current song went so playback doesn't skip
        if (app->current_track_idx != -1) {
            for(size_t i=0; i<app->play_order.size(); i++) {
                if(app->play_order[i] == current_real_idx) {
                    app->current_track_idx = i;
                    break;
                }
            }
        }
    } else {
        // If turning OFF, restore sequential order (0, 1, 2...)
        size_t current_real_idx = app->play_order[app->current_track_idx];
        std::iota(app->play_order.begin(), app->play_order.end(), 0);
        app->current_track_idx = current_real_idx;
    }
}

void PlaylistManager::toggleRepeat() {
    // Cycle: Off (0) -> All (2) -> One (1) -> Off (0)
    // Note: I prefer All before One as it's more common
    if (app->repeatMode == REP_OFF) app->repeatMode = REP_ALL;
    else if (app->repeatMode == REP_ALL) app->repeatMode = REP_ONE;
    else app->repeatMode = REP_OFF;
}

// --- FILE OPERATIONS ---
void PlaylistManager::addFiles() {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Add Music", GTK_WINDOW(parentWindow), GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Audio & Playlists");
    gtk_file_filter_add_pattern(filter, "*"); 
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        GSList *filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        size_t oldSize = app->playlist.size();
        for (GSList *iter = filenames; iter; iter = iter->next) {
            char *cpath = (char *)iter->data;
            std::string path(cpath);
            // Simple M3U check
            if (path.length() > 4 && (path.substr(path.length()-4) == ".m3u" || path.substr(path.length()-4) == ".M3U")) {
                std::string dir = path.substr(0, path.find_last_of("/")+1);
                std::ifstream f(path); std::string l;
                while(std::getline(f, l)) {
                    if(l.empty() || l[0]=='#') continue;
                    l.erase(0, l.find_first_not_of(" \r")); l.erase(l.find_last_not_of(" \r")+1);
                    if(l[0] == '/') app->playlist.push_back(l);
                    else app->playlist.push_back(dir + l);
                }
            } else {
                app->playlist.push_back(path);
            }
            g_free(cpath);
        }
        g_slist_free(filenames);
        
        // Expand order
        size_t newSize = app->playlist.size();
        app->play_order.resize(newSize);
        // Only fill new slots sequentially to respect existing shuffle if we wanted to be complex,
        // but for now we reset order to keep it simple or just append
        for(size_t i = oldSize; i < newSize; i++) app->play_order[i] = i;
        
        refreshUI();
    }
    gtk_widget_destroy(dialog);
}

void PlaylistManager::clear() {
    player->stop();
    app->playlist.clear();
    app->play_order.clear();
    app->current_track_idx = -1;
    refreshUI();
}

void PlaylistManager::refreshUI() {
    GList *children = gtk_container_get_children(GTK_CONTAINER(listBox));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) gtk_widget_destroy(GTK_WIDGET(iter->data));
    g_list_free(children);

    for (size_t i = 0; i < app->playlist.size(); i++) {
        std::string path = app->playlist[i];
        std::string name = path.substr(path.find_last_of("/") + 1);
        std::string labelStr = std::to_string(i + 1) + ". " + name;
        GtkWidget* label = gtk_label_new(labelStr.c_str());
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_container_add(GTK_CONTAINER(listBox), label);
        gtk_widget_show(label);
    }
}

void PlaylistManager::onRowActivated(GtkListBox* box, GtkListBoxRow* row) {
    int visual_index = gtk_list_box_row_get_index(row);
    if (visual_index < 0) return;

    if (!app->shuffle) {
        app->current_track_idx = visual_index;
    } else {
        for(size_t i=0; i<app->play_order.size(); i++) {
            if((int)app->play_order[i] == visual_index) {
                app->current_track_idx = i;
                break;
            }
        }
    }
    size_t real_idx = app->play_order[app->current_track_idx];
    player->load(app->playlist[real_idx]);
    player->play();
}

void PlaylistManager::playNext() {
    if (app->playlist.empty()) return;
    int next = app->current_track_idx + 1;
    if (next >= (int)app->play_order.size()) next = 0;
    app->current_track_idx = next;
    player->load(app->playlist[app->play_order[next]]);
    player->play();
    highlightCurrentTrack();
}

void PlaylistManager::playPrev() {
    if (app->playlist.empty()) return;
    if (player->getPosition() > 2.0) {
        player->load(app->playlist[app->play_order[app->current_track_idx]]);
        player->play();
        return;
    }
    int prev = app->current_track_idx - 1;
    if (prev < 0) prev = app->play_order.size() - 1;
    app->current_track_idx = prev;
    player->load(app->playlist[app->play_order[prev]]);
    player->play();
    highlightCurrentTrack();
}

void PlaylistManager::autoAdvance() {
    if (app->playlist.empty()) { player->stop(); return; }
    if (app->repeatMode == REP_ONE) {
        player->load(app->playlist[app->play_order[app->current_track_idx]]);
        player->play();
        return;
    }
    int next = app->current_track_idx + 1;
    if (next >= (int)app->play_order.size()) {
        if (app->repeatMode == REP_ALL) next = 0;
        else { player->stop(); return; }
    }
    app->current_track_idx = next;
    player->load(app->playlist[app->play_order[next]]);
    player->play();
    highlightCurrentTrack();
}

void PlaylistManager::selectNext() {
    GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(listBox));
    if (row) {
        int idx = gtk_list_box_row_get_index(row);
        if (idx < (int)app->playlist.size() - 1) 
            gtk_list_box_select_row(GTK_LIST_BOX(listBox), gtk_list_box_get_row_at_index(GTK_LIST_BOX(listBox), idx + 1));
    }
}
void PlaylistManager::selectPrev() {
    GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(listBox));
    if (row) {
        int idx = gtk_list_box_row_get_index(row);
        if (idx > 0) 
            gtk_list_box_select_row(GTK_LIST_BOX(listBox), gtk_list_box_get_row_at_index(GTK_LIST_BOX(listBox), idx - 1));
    }
}
void PlaylistManager::deleteSelected() {
     GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(listBox));
     if(row) {
         int idx = gtk_list_box_row_get_index(row);
         app->playlist.erase(app->playlist.begin() + idx);
         app->play_order.clear();
         for(size_t i=0; i<app->playlist.size(); i++) app->play_order.push_back(i); // reset shuffle on delete for safety
         app->current_track_idx = -1;
         player->stop();
         refreshUI();
     }
}
