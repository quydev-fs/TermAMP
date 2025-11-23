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

void PlaylistManager::addFiles() {
    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;

    dialog = gtk_file_chooser_dialog_new("Add Music",
                                         GTK_WINDOW(parentWindow),
                                         action,
                                         "_Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         "_Open",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);
    
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Audio & Playlists");
    gtk_file_filter_add_pattern(filter, "*.mp3");
    gtk_file_filter_add_pattern(filter, "*.wav");
    gtk_file_filter_add_pattern(filter, "*.ogg");
    gtk_file_filter_add_pattern(filter, "*.m3u");
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
                // FIX: Resolve relative paths based on M3U location
                std::string m3uDir = "";
                size_t lastSlash = path.find_last_of("/");
                if (lastSlash != std::string::npos) {
                    m3uDir = path.substr(0, lastSlash + 1);
                }

                std::ifstream file(path);
                std::string line;
                while (std::getline(file, line)) {
                    // Trim
                    line.erase(0, line.find_first_not_of(" \t\r\n"));
                    line.erase(line.find_last_not_of(" \t\r\n") + 1);
                    
                    if (line.empty() || line[0] == '#') continue;
                    
                    // Check if absolute or relative
                    if (line.length() > 0 && line[0] == '/') {
                        app->playlist.push_back(line);
                    } else {
                        // Prepend M3U directory to relative path
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
        
        refreshUI();
    }

    gtk_widget_destroy(dialog);
}

void PlaylistManager::clear() {
    player->stop();
    app->playlist.clear();
    app->play_order.clear();
    app->current_track_idx = -1;
    app->playing = false;
    app->paused = false;
    refreshUI();
}

void PlaylistManager::refreshUI() {
    GList *children, *iter;
    children = gtk_container_get_children(GTK_CONTAINER(listBox));
    for (iter = children; iter != NULL; iter = g_list_next(iter)) {
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
        
        if (app->current_track_idx != -1 && 
            (int)app->play_order[app->current_track_idx] == (int)i) {
            // CSS handles selection, but logic is here if needed
        }
        
        gtk_container_add(GTK_CONTAINER(listBox), label);
        gtk_widget_show(label);
    }
}

void PlaylistManager::onRowActivated(GtkListBox* box, GtkListBoxRow* row) {
    int visual_index = gtk_list_box_row_get_index(row);
    if (visual_index >= 0 && visual_index < (int)app->playlist.size()) {
        
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
}

void PlaylistManager::selectNext() {
    GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(listBox));
    if (row) {
        int idx = gtk_list_box_row_get_index(row);
        if (idx < (int)app->playlist.size() - 1) {
            GtkListBoxRow* nextRow = gtk_list_box_get_row_at_index(GTK_LIST_BOX(listBox), idx + 1);
            gtk_list_box_select_row(GTK_LIST_BOX(listBox), nextRow);
        }
    }
}

void PlaylistManager::selectPrev() {
    GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(listBox));
    if (row) {
        int idx = gtk_list_box_row_get_index(row);
        if (idx > 0) {
            GtkListBoxRow* prevRow = gtk_list_box_get_row_at_index(GTK_LIST_BOX(listBox), idx - 1);
            gtk_list_box_select_row(GTK_LIST_BOX(listBox), prevRow);
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
