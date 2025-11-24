#include "playlist.h"
#include <iostream>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <random>
#include <chrono>

// --- CONSTRUCTOR ---
PlaylistManager::PlaylistManager(AppState* state, Player* pl, GtkWidget* list) 
    : app(state), player(pl), listBox(list) {
    parentWindow = gtk_widget_get_toplevel(listBox);
}

// --- HELPER: Highlight ---
void PlaylistManager::highlightCurrentTrack() {
    if (app->current_track_idx < 0 || app->current_track_idx >= (int)app->play_order.size()) return;
    int actual_playlist_index = app->play_order[app->current_track_idx];
    GtkListBoxRow* row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(listBox), actual_playlist_index);
    if (row) {
        gtk_list_box_select_row(GTK_LIST_BOX(listBox), row);
    }
}

// --- FILE CHOOSER ---
void PlaylistManager::addFiles() {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Add Music",
                                         GTK_WINDOW(parentWindow),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_Open", GTK_RESPONSE_ACCEPT,
                                         NULL);
    
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

    // Create Filter
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Supported Audio");

    // Tier 1
    gtk_file_filter_add_pattern(filter, "*.mp3"); gtk_file_filter_add_pattern(filter, "*.MP3");
    gtk_file_filter_add_pattern(filter, "*.wav"); gtk_file_filter_add_pattern(filter, "*.WAV");
    gtk_file_filter_add_pattern(filter, "*.ogg"); gtk_file_filter_add_pattern(filter, "*.OGG");
    gtk_file_filter_add_pattern(filter, "*.flac"); gtk_file_filter_add_pattern(filter, "*.FLAC");
    gtk_file_filter_add_pattern(filter, "*.m4a"); gtk_file_filter_add_pattern(filter, "*.M4A");
    gtk_file_filter_add_pattern(filter, "*.aac"); gtk_file_filter_add_pattern(filter, "*.AAC");
    gtk_file_filter_add_pattern(filter, "*.opus"); gtk_file_filter_add_pattern(filter, "*.OPUS");

    // Tier 2
    gtk_file_filter_add_pattern(filter, "*.wma"); gtk_file_filter_add_pattern(filter, "*.WMA");
    gtk_file_filter_add_pattern(filter, "*.ape"); gtk_file_filter_add_pattern(filter, "*.APE");
    gtk_file_filter_add_pattern(filter, "*.alac"); gtk_file_filter_add_pattern(filter, "*.ALAC");
    gtk_file_filter_add_pattern(filter, "*.mka"); gtk_file_filter_add_pattern(filter, "*.MKA");

    // Tier 3
    gtk_file_filter_add_pattern(filter, "*.mod"); 
    gtk_file_filter_add_pattern(filter, "*.xm");  
    gtk_file_filter_add_pattern(filter, "*.it");  
    gtk_file_filter_add_pattern(filter, "*.s3m"); 
    gtk_file_filter_add_pattern(filter, "*.mid"); 
    gtk_file_filter_add_pattern(filter, "*.midi");
    gtk_file_filter_add_pattern(filter, "*.dsd"); 
    gtk_file_filter_add_pattern(filter, "*.dsf");

    // Playlists
    gtk_file_filter_add_pattern(filter, "*.m3u");
    gtk_file_filter_add_pattern(filter, "*.M3U");

    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        GSList *filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        GSList *iter;
        
        size_t oldSize = app->playlist.size();
        
        for (iter = filenames; iter; iter = iter->next) {
            char *cpath = (char *)iter->data;
            std::string path(cpath);
            
            bool isM3u = false;
            if (path.length() > 4) {
                std::string ext = path.substr(path.length() - 4);
                if (ext == ".m3u" || ext == ".M3U") isM3u = true;
            }

            if (isM3u) {
                std::string m3uDir = "";
                size_t lastSlash = path.find_last_of("/");
                if (lastSlash != std::string::npos) {
                    m3uDir = path.substr(0, lastSlash + 1);
                }

                std::ifstream file(path);
                std::string line;
                while (std::getline(file, line)) {
                    line.erase(0, line.find_first_not_of(" \t\r\n"));
                    line.erase(line.find_last_not_of(" \t\r\n") + 1);
                    if (line.empty() || line[0] == '#') continue;
                    
                    if (line.length() > 0 && line[0] == '/') {
                        app->playlist.push_back(line);
                    } else {
                        app->playlist.push_back(m3uDir + line);
                    }
                }
            } else {
                app->playlist.push_back(path);
            }
            g_free(cpath);
        }
        g_slist_free(filenames);
        
        size_t newSize = app->playlist.size();
        app->play_order.resize(newSize);
        for(size_t i = oldSize; i < newSize; i++) {
            app->play_order[i] = i;
        }
        
        // If shuffling was already on, we might want to re-shuffle or just append
        // For now, we just append linearly to keep it simple.
        
        refreshUI();
    }

    gtk_widget_destroy(dialog);
}

// --- CLEAR ---
void PlaylistManager::clear() {
    player->stop();
    app->playlist.clear();
    app->play_order.clear();
    app->current_track_idx = -1;
    app->playing = false;
    app->paused = false;
    refreshUI();
}

// --- REFRESH UI ---
void PlaylistManager::refreshUI() {
    GList *children = gtk_container_get_children(GTK_CONTAINER(listBox));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
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

// --- ROW CLICK ---
void PlaylistManager::onRowActivated(GtkListBox* box, GtkListBoxRow* row) {
    int visual_index = gtk_list_box_row_get_index(row);
    if (visual_index < 0 || visual_index >= (int)app->playlist.size()) return;

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

    size_t real_file_index = app->play_order[app->current_track_idx];
    player->load(app->playlist[real_file_index]);
    player->play();
}

// --- AUTO ADVANCE ---
void PlaylistManager::autoAdvance() {
    if (app->playlist.empty()) {
        player->stop();
        return;
    }
    if (app->repeatMode == REP_ONE) {
        if (app->current_track_idx >= 0) {
            size_t real_idx = app->play_order[app->current_track_idx];
            player->load(app->playlist[real_idx]);
            player->play();
        }
        return;
    }
    int next = app->current_track_idx + 1;
    if (next >= (int)app->play_order.size()) {
        if (app->repeatMode == REP_ALL) {
            next = 0;
        } else {
            player->stop();
            app->current_track_idx = 0;
            highlightCurrentTrack();
            return;
        }
    }
    app->current_track_idx = next;
    size_t real_idx = app->play_order[app->current_track_idx];
    player->load(app->playlist[real_idx]);
    player->play();
    highlightCurrentTrack();
}

// --- CONTROLS ---
void PlaylistManager::playNext() {
    if (app->playlist.empty()) return;
    int next = app->current_track_idx + 1;
    if (next >= (int)app->play_order.size()) next = 0; 
    app->current_track_idx = next;
    size_t real_idx = app->play_order[app->current_track_idx];
    player->load(app->playlist[real_idx]);
    player->play();
    highlightCurrentTrack();
}

void PlaylistManager::playPrev() {
    if (app->playlist.empty()) return;
    if (player->getPosition() > 2.0) {
        if(app->current_track_idx >= 0) {
            player->load(app->playlist[app->play_order[app->current_track_idx]]); 
            player->play();
        }
        return;
    }
    int prev = app->current_track_idx - 1;
    if (prev < 0) prev = app->play_order.size() - 1; 
    app->current_track_idx = prev;
    size_t real_idx = app->play_order[app->current_track_idx];
    player->load(app->playlist[real_idx]);
    player->play();
    highlightCurrentTrack();
}

// --- FIXED: STATE TOGGLES (CRASH FIX) ---
void PlaylistManager::toggleShuffle() {
    app->shuffle = !app->shuffle;
    
    // GUARD CLAUSE: If playlist is empty, stop here. 
    // We toggled the bool so the UI button will turn green (which is fine),
    // but we MUST NOT touch the vectors.
    if (app->playlist.empty()) return;

    if (app->shuffle) {
        // Turning ON Shuffle
        size_t current_real_idx = 0;
        if (app->current_track_idx != -1) {
            current_real_idx = app->play_order[app->current_track_idx];
        }

        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::shuffle(app->play_order.begin(), app->play_order.end(), std::default_random_engine(seed));

        // Map current song to new index
        if (app->current_track_idx != -1) {
            for(size_t i=0; i<app->play_order.size(); i++) {
                if(app->play_order[i] == current_real_idx) {
                    app->current_track_idx = i;
                    break;
                }
            }
        }
    } else {
        // Turning OFF Shuffle
        
        // GUARD: Ensure we actually have a valid track selected
        if (app->current_track_idx != -1 && app->current_track_idx < (int)app->play_order.size()) {
            // Save the Real ID of song playing
            size_t current_real_idx = app->play_order[app->current_track_idx];
            
            // Reset order
            std::iota(app->play_order.begin(), app->play_order.end(), 0);
            
            // Set index to Real ID (since order is now 0,1,2...)
            app->current_track_idx = current_real_idx;
        } else {
            // Nothing playing, just reset order
            std::iota(app->play_order.begin(), app->play_order.end(), 0);
            // current_track_idx remains -1
        }
    }
}

void PlaylistManager::toggleRepeat() {
    if (app->repeatMode == REP_OFF) app->repeatMode = REP_ALL;
    else if (app->repeatMode == REP_ALL) app->repeatMode = REP_ONE;
    else app->repeatMode = REP_OFF;
}

// --- KEYBOARD HELPERS ---
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
         app->play_order.clear();
         app->play_order.resize(app->playlist.size());
         std::iota(app->play_order.begin(), app->play_order.end(), 0);
         app->current_track_idx = -1; 
         player->stop();
         refreshUI();
     }
}
