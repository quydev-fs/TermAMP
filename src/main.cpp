#include "ui.h"
#include "player.h"
#include "common.h"
#include "playlist.h"
#include <iostream>

int main(int argc, char** argv) {
    std::cout << "Starting TermuxMusic95..." << std::endl;

    AppState appState;
    
    // Only try to load if arguments are present
    if (argc >= 2) {
        loadPlaylist(appState, argc, argv);
    }

    // Removed the block that returned 1 if empty.
    // Now it just starts empty.

    UI ui(&appState);
    if (!ui.init()) {
        std::cerr << "Failed to initialize X11 Display." << std::endl;
        return 1;
    }

    Player player(&appState);
    player.start();

    ui.runLoop();

    player.stop();
    return 0;
}
