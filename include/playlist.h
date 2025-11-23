#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "common.h"
#include "player.h"

class PlaylistManager {
public:
    PlaylistManager(AppState* state, Player* player, GtkWidget* listBox);
    
    void addFiles();
    void clear();
    void refreshUI();
    
    // List Interaction
    void onRowActivated(GtkListBox* box, GtkListBoxRow* row);
    
    // Keyboard Selection (Moves highlight only)
    void selectNext();
    void selectPrev();
    void deleteSelected();

    // Playback Controls (Actually changes the song)
    void playNext();
    void playPrev();

private:
    // Helper to sync UI highlight with actual playing track
    void highlightCurrentTrack();

    AppState* app;
    Player* player;
    GtkWidget* listBox; 
    GtkWidget* parentWindow;
};

#endif
