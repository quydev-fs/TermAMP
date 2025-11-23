#include "playlist.h"
#include <iostream>
#include <fstream>
#include <numeric>
#include <algorithm>

PlaylistManager::PlaylistManager(AppState* state, Player* pl, GtkWidget* list) 
    : app(state), player(pl), listBox(list) {
    parentWindow = gtk_widget_get_toplevel(listBox);
}

// ... [addFiles, clear, refreshUI code remains the same as previous step] ...
// ... [Paste the addFiles/clear/refreshUI code from the previous working version here] ...
// For brevity, I will focus on the NEW logic below. 
// Be sure to keep addFiles/clear/refreshUI!

void PlaylistManager::addFiles() {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Add Music", GTK_WINDOW(parentWindow), GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Audio");
    gtk_file_filter_add_pattern(filter, "*"); // Allow all for simplicity, or restrict
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        GSList *filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        size_t oldSize = app->playlist.size();
        for (GSList *iter = filenames; iter; iter = iter->next) {
            char *cpath = (char *)iter->data;
            app->playlist.push_back(std::string(cpath));
            g_free(cpath);
        }
        g_slist_free(filenames);
        size_t newSize = app->playlist.size();
        app->play_order.resize(newSize);
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
        size_t lastSlash = path.find_last_of("/");
        std::string name = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
        std::string labelStr = std::to_string(i + 1) + ". " + name;
        GtkWidget* label = gtk_label_new(labelStr.c_str());
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_container_add(GTK_CONTAINER(listBox), label);
        gtk_widget_show(label);
    }
}

void PlaylistManager::highlightCurrentTrack() {
    // Find the visual row index corresponding to the current playing track
    // In default mode: visual index == app->current_track_idx (which maps to play_order)
    // Wait, app->current_track_idx IS the index in play_order.
    // We need to find which visual row matches app->play_order[app->current_track_idx]
    
    if (app->current_track_idx < 0 || app->current_track_idx >= (int)app->play_order.size()) return;

    int actual_playlist_index = app->play_order[app->current_track_idx];
    
    // In this simple list, row index = playlist index
    GtkListBoxRow* row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(listBox), actual_playlist_index);
    if (row) {
        gtk_list_box_select_row(GTK_LIST_BOX(listBox), row);
        // Optional: scroll to row?
    }
}

// --- USER CLICKED A ROW ---
void PlaylistManager::onRowActivated(GtkListBox* box, GtkListBoxRow* row) {
    int visual_index = gtk_list_box_row_get_index(row);
    if (visual_index < 0) return;

    // Update State
    if (!app->shuffle) {
        app->current_track_idx = visual_index;
    } else {
        // Find this song in the shuffle map
        for(size_t i=0; i<app->play_order.size(); i++) {
            if((int)app->play_order[i] == visual_index) {
                app->current_track_idx = i;
                break;
            }
        }
    }

    // Play
    size_t real_file_index = app->play_order[app->current_track_idx];
    player->load(app->playlist[real_file_index]);
    player->play();
}

// --- BUTTON: NEXT TRACK ---
void PlaylistManager::playNext() {
    if (app->playlist.empty()) return;

    int next = app->current_track_idx + 1;
    if (next >= (int)app->play_order.size()) {
        next = 0; // Wrap around (or stop if Repeat Off, handled in player)
    }
    
    app->current_track_idx = next;
    
    // Load & Play
    size_t real_idx = app->play_order[app->current_track_idx];
    player->load(app->playlist[real_idx]);
    player->play();
    
    highlightCurrentTrack();
}

// --- BUTTON: PREV TRACK ---
void PlaylistManager::playPrev() {
    if (app->playlist.empty()) return;

    // Smart Previous: If playing > 2s, restart song
    if (player->getPosition() > 2.0) {
        player->load(app->playlist[app->play_order[app->current_track_idx]]); // Reload same
        player->play();
        return;
    }

    // Go back
    int prev = app->current_track_idx - 1;
    if (prev < 0) {
        prev = app->play_order.size() - 1; // Wrap to end
    }
    
    app->current_track_idx = prev;
    
    size_t real_idx = app->play_order[app->current_track_idx];
    player->load(app->playlist[real_idx]);
    player->play();
    
    highlightCurrentTrack();
}

// --- KEYBOARD: SELECTION ONLY ---
void PlaylistManager::selectNext() {
    GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(listBox));
    if (row) {
        int idx = gtk_list_box_row_get_index(row);
        if (idx < (int)app->playlist.size() - 1) {
            GtkListBoxRow* next = gtk_list_box_get_row_at_index(GTK_LIST_BOX(listBox), idx + 1);
            gtk_list_box_select_row(GTK_LIST_BOX(listBox), next);
        }
    }
}

void PlaylistManager::selectPrev() {
    GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(listBox));
    if (row) {
        int idx = gtk_list_box_row_get_index(row);
        if (idx > 0) {
            GtkListBoxRow* prev = gtk_list_box_get_row_at_index(GTK_LIST_BOX(listBox), idx - 1);
            gtk_list_box_select_row(GTK_LIST_BOX(listBox), prev);
        }
    }
}

void PlaylistManager::deleteSelected() {
     GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(listBox));
     if(row) {
         int idx = gtk_list_box_row_get_index(row);
         app->playlist.erase(app->playlist.begin() + idx);
         // Quick re-indexing for stability
         app->play_order.clear();
         for(size_t i=0; i<app->playlist.size(); i++) app->play_order.push_back(i);
         
         app->current_track_idx = -1;
         player->stop();
         refreshUI();
     }
}
