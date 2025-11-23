#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "common.h"
#include "player.h"

class PlaylistManager {
public:
    PlaylistManager(AppState* state, Player* player, GtkWidget* listBox);
    
    // File Ops
    void addFiles();
    void clear();
    void refreshUI();
    
    // Controls
    void onRowActivated(GtkListBox* box, GtkListBoxRow* row);
    void selectNext();
    void selectPrev();
    void deleteSelected();
    void playNext();
    void playPrev();
    void autoAdvance();

    // NEW: State Toggles
    void toggleShuffle();
    void toggleRepeat();

private:
    void highlightCurrentTrack();

    AppState* app;
    Player* player;
    GtkWidget* listBox; 
    GtkWidget* parentWindow;
};

#endif
