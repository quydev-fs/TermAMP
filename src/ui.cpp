#include "ui.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
      
// --- CONSTANTS ---      
const int FULL_WIDTH = 320;      
const int FULL_HEIGHT_INIT = 360;       
const int VISUALIZER_FULL_HEIGHT = 40;       
const int MINI_HEIGHT_REPURPOSED = 180;       
      
UI::UI(int argc, char** argv) {      
    gtk_init(&argc, &argv);      
    player = nullptr;      
    playlistMgr = nullptr;      
    visualizer = nullptr;      
    playlistBox = nullptr;      
    drawingArea = nullptr;      
          
    // REFACTOR: Load resources via Utils      
    Utils::loadGlobalCSS();      
}      
      
UI::~UI() {      
    if (playlistBox) g_object_unref(playlistBox);      
    if (drawingArea) g_object_unref(drawingArea);      
    if (playlistMgr) delete playlistMgr;      
    if (visualizer) delete visualizer;      
    if (player) delete player;      
}      
      
// --- NEW: ABOUT DIALOG ---      
void UI::onAboutClicked(GtkMenuItem* item, gpointer data) {      
    UI* ui = (UI*)data;      
          
    GtkWidget* dialog = gtk_dialog_new_with_buttons(      
        "About TermAMP",      
        GTK_WINDOW(ui->window),      
        (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),      
        "Close", GTK_RESPONSE_OK,      
        NULL      
    );      
          
    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));      
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 15);      
          
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);      
    gtk_container_add(GTK_CONTAINER(content_area), box);      
          
    GtkWidget* img = Utils::createLogoImage(64);      
    gtk_widget_set_halign(img, GTK_ALIGN_CENTER);      
    gtk_box_pack_start(GTK_BOX(box), img, FALSE, FALSE, 0);      
          
    GtkWidget* lblName = gtk_label_new(NULL);      
    gtk_label_set_markup(GTK_LABEL(lblName), "<span size='x-large' weight='bold' color='#000'>TermAMP</span>");      
    gtk_box_pack_start(GTK_BOX(box), lblName, FALSE, FALSE, 0);      
          
    GtkWidget* lblCredit = gtk_label_new("(c) quydev-fs 2025\nLicensed under MIT");      
    gtk_label_set_justify(GTK_LABEL(lblCredit), GTK_JUSTIFY_CENTER);      
    GtkStyleContext *context = gtk_widget_get_style_context(lblCredit);      
    gtk_style_context_add_class(context, "dim-label");       
    gtk_box_pack_start(GTK_BOX(box), lblCredit, FALSE, FALSE, 0);      
          
    GtkWidget* lblQuote = gtk_label_new(NULL);      
    gtk_label_set_markup(GTK_LABEL(lblQuote), "<i>\"it may better when it come to retro\"</i>");      
    gtk_box_pack_start(GTK_BOX(box), lblQuote, FALSE, FALSE, 5);      
      
    gtk_widget_show_all(dialog);      
    gtk_dialog_run(GTK_DIALOG(dialog));      
    gtk_widget_destroy(dialog);      
}      
      
// --- MINI MODE LOGIC ---      
void UI::toggleMiniMode(bool force_resize) {      
    is_mini_mode = !is_mini_mode;      
      
    GtkWidget* scrolled = gtk_widget_get_parent(playlistBox);      
    if (!scrolled) scrolled = gtk_widget_get_parent(drawingArea);      
      
    if (is_mini_mode) {      
        g_object_ref(drawingArea);      
        g_object_ref(playlistBox);      
      
        gtk_container_remove(GTK_CONTAINER(visualizerContainerBox), drawingArea);      
        gtk_widget_hide(visualizerContainerBox);       
      
        gtk_container_remove(GTK_CONTAINER(scrolled), playlistBox);      
        gtk_container_add(GTK_CONTAINER(scrolled), drawingArea);      
        gtk_widget_set_size_request(drawingArea, FULL_WIDTH, 120);       
        gtk_widget_show(drawingArea);      
      
        gtk_window_set_default_size(GTK_WINDOW(window), FULL_WIDTH, MINI_HEIGHT_REPURPOSED);      
        gtk_window_resize(GTK_WINDOW(window), FULL_WIDTH, MINI_HEIGHT_REPURPOSED);      
        gtk_button_set_label(GTK_BUTTON(btnMiniMode), "F");      
        g_object_unref(drawingArea);      
        g_object_unref(playlistBox);      
    } else {      
        g_object_ref(drawingArea);      
        g_object_ref(playlistBox);      
      
        gtk_container_remove(GTK_CONTAINER(scrolled), drawingArea);      
        gtk_container_add(GTK_CONTAINER(scrolled), playlistBox);      
        gtk_widget_show(playlistBox);      
              
        gtk_box_pack_start(GTK_BOX(visualizerContainerBox), drawingArea, FALSE, FALSE, 0);      
        gtk_box_reorder_child(GTK_BOX(visualizerContainerBox), drawingArea, 0);      
              
        gtk_widget_set_size_request(drawingArea, -1, VISUALIZER_FULL_HEIGHT);      
        gtk_widget_show(visualizerContainerBox);      
        gtk_widget_show(drawingArea);      
      
        gtk_window_set_default_size(GTK_WINDOW(window), FULL_WIDTH, FULL_HEIGHT_INIT);      
        gtk_window_resize(GTK_WINDOW(window), FULL_WIDTH, FULL_HEIGHT_INIT);      
        gtk_button_set_label(GTK_BUTTON(btnMiniMode), "M");      
        g_object_unref(drawingArea);      
        g_object_unref(playlistBox);      
    }      
}      
      
void UI::onMiniModeClicked(GtkButton* btn, gpointer data) { ((UI*)data)->toggleMiniMode(); }      
void UI::onPlayClicked(GtkButton* b, gpointer d) {       
    UI* ui = (UI*)d;      
    if (ui->appState.playlist.empty()) return;      
          
    GstState state = ui->player->getState();      
    if (state == GST_STATE_PAUSED) {      
        ui->player->play();      
        return;      
    }      
    if (state == GST_STATE_PLAYING) return;      
      
    if (ui->appState.current_track_idx == -1) {      
        ui->appState.current_track_idx = 0;      
        size_t idx = (!ui->appState.play_order.empty()) ? ui->appState.play_order[0] : 0;      
        ui->player->load(ui->appState.playlist[idx]);      
    } else {      
        size_t idx = ui->appState.play_order[ui->appState.current_track_idx];      
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

// Helper function to update album art display
void UI::updateAlbumArt() {
    // First, free the previous album art if it exists
    if (appState.current_album_art) {
        g_object_unref(appState.current_album_art);
        appState.current_album_art = nullptr;
    }

    // Try to extract album art for the current track
    if (!appState.playlist.empty() && appState.current_track_idx >= 0 &&
        appState.current_track_idx < (int)appState.play_order.size()) {

        size_t real_idx = appState.play_order[appState.current_track_idx];
        if (real_idx < appState.playlist.size()) {
            std::string track_path = appState.playlist[real_idx];

            // Try to extract album art from the audio file or its directory
            appState.current_album_art = Utils::loadAlbumArtFromCache(track_path, 150);

            if (appState.current_album_art) {
                // Update the album art image widget
                gtk_image_set_from_pixbuf(GTK_IMAGE(albumArtImage), appState.current_album_art);
                gtk_widget_show(albumArtImage); // Make sure it's visible
            } else {
                // If no album art found, hide the image widget or show a default
                gtk_image_set_from_icon_name(GTK_IMAGE(albumArtImage), "audio-x-generic", GTK_ICON_SIZE_DIALOG);
                gtk_widget_hide(albumArtImage); // Hide if no art
            }
        }
    }
}

// --- EQUALIZER HANDLERS ---
void UI::onEqToggled(GtkToggleButton* toggle, gpointer data) {
    UI* ui = (UI*)data;
    bool enabled = gtk_toggle_button_get_active(toggle);
    ui->player->enableEqualizer(enabled);

    // Show/hide equalizer controls based on toggle state
    GtkWidget* box = gtk_widget_get_parent(ui->eqBandsBox);
    if (box) {
        if (enabled) gtk_widget_show(box);
        else gtk_widget_hide(box);
    }
}

void UI::onEqBandChanged(GtkRange* range, gpointer data) {
    UI* ui = (UI*)data;

    // Find which slider was changed by comparing with stored sliders
    for (size_t i = 0; i < ui->eqSliders.size(); ++i) {
        if (ui->eqSliders[i] == GTK_WIDGET(range)) {
            double value = gtk_range_get_value(range);
            ui->player->setEqualizerBand(i, value);
            break;
        }
    }
}

void UI::onEqPresetChanged(GtkComboBox* combo, gpointer data) {
    UI* ui = (UI*)data;
    int active = gtk_combo_box_get_active(combo);

    // Define presets: rock, pop, classical, jazz, bass
    std::vector<std::vector<double>> presets = {
        {4.0, 2.0, 0.0, -2.0, -4.0, -2.0, 0.0, 2.0, 4.0, 2.0},  // Rock
        {0.0, 0.0, 2.0, 4.0, 4.0, 4.0, 2.0, 0.0, 0.0, 0.0},    // Pop
        {4.0, 4.0, 2.0, 0.0, -2.0, -4.0, -4.0, -4.0, -2.0, 0.0}, // Classical
        {2.0, 0.0, -2.0, 0.0, 2.0, 4.0, 2.0, 0.0, -2.0, 0.0},   // Jazz
        {-4.0, -2.0, 0.0, 2.0, 4.0, 4.0, 2.0, 0.0, -2.0, -4.0}  // Bass
    };

    if (active >= 0 && active < (int)presets.size()) {
        ui->player->setEqualizerBands(presets[active]);

        // Update slider values to match preset
        for (size_t i = 0; i < std::min(presets[active].size(), ui->eqSliders.size()); ++i) {
            g_signal_handlers_block_by_func(ui->eqSliders[i], (void*)onEqBandChanged, ui);
            gtk_range_set_value(GTK_RANGE(ui->eqSliders[i]), presets[active][i]);
            g_signal_handlers_unblock_by_func(ui->eqSliders[i], (void*)onEqBandChanged, ui);
        }
    }
}

void UI::onEqResetClicked(GtkButton* btn, gpointer data) {
    UI* ui = (UI*)data;

    // Reset all bands to 0.0 (flat response)
    std::vector<double> flatBands(10, 0.0);
    ui->player->setEqualizerBands(flatBands);

    // Update slider values to 0.0
    for (size_t i = 0; i < ui->eqSliders.size(); ++i) {
        g_signal_handlers_block_by_func(ui->eqSliders[i], (void*)onEqBandChanged, ui);
        gtk_range_set_value(GTK_RANGE(ui->eqSliders[i]), 0.0);
        g_signal_handlers_unblock_by_func(ui->eqSliders[i], (void*)onEqBandChanged, ui);
    }
}

// --- CROSSFADING HANDLERS ---
void UI::onCrossfadeToggled(GtkToggleButton* toggle, gpointer data) {
    UI* ui = (UI*)data;
    bool enabled = gtk_toggle_button_get_active(toggle);
    ui->appState.crossfading_enabled = enabled;

    // Update the sensitivity of the duration slider based on toggle state
    gtk_widget_set_sensitive(ui->crossfadeDurationScale, enabled);
}

void UI::onCrossfadeDurationChanged(GtkRange* range, gpointer data) {
    UI* ui = (UI*)data;
    double value = gtk_range_get_value(range);
    ui->appState.crossfade_duration = value;

    // Update the label to show current value
    char text[20];
    sprintf(text, "%.1f s", value);
    gtk_label_set_text(GTK_LABEL(ui->crossfadeDurationLabel), text);
}

// --- AUDIO CONVERSION HANDLERS ---
void UI::onConvertAudioClicked(GtkButton* btn, gpointer data) {
    UI* ui = (UI*)data;

    // Get the currently selected track in the playlist
    GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(ui->playlistBox));
    if (!row) {
        std::cerr << "No track selected for conversion" << std::endl;
        return;
    }

    int selected_index = gtk_list_box_row_get_index(row);
    if (selected_index < 0 || selected_index >= (int)ui->appState.playlist.size()) {
        std::cerr << "Invalid playlist index" << std::endl;
        return;
    }

    // Get the selected format
    int format_index = gtk_combo_box_get_active(GTK_COMBO_BOX(ui->convertFormatCombo));
    if (format_index < 0) {
        std::cerr << "No format selected for conversion" << std::endl;
        return;
    }

    // Get the output format from the combo box
    gchar* format_text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(ui->convertFormatCombo));
    if (!format_text) {
        std::cerr << "Failed to get selected format" << std::endl;
        return;
    }

    std::string input_path = ui->appState.playlist[selected_index];
    std::string output_format = format_text;
    g_free(format_text);

    // Generate output path (change extension)
    std::string output_path = input_path;
    size_t last_dot = output_path.find_last_of(".");
    if (last_dot != std::string::npos) {
        output_path = output_path.substr(0, last_dot + 1) + output_format;
    } else {
        output_path += "." + output_format;
    }

    // Disable convert button during conversion
    gtk_widget_set_sensitive(ui->convertBtn, FALSE);

    // Show progress bar
    gtk_widget_show(ui->convertProgress);

    // Perform the conversion in a separate thread
    ui->appState.conversion_in_progress = true;

    std::thread conversion_thread([ui, input_path, output_path, output_format]() {
        // Simple progress simulation since actual progress is difficult with GStreamer pipelines
        bool success = Utils::convertAudioFormat(input_path, output_path, output_format);

        // Store the result
        std::string result_msg = success ? "Conversion completed: " + output_path : "Conversion failed!";

        // Update UI after conversion
        struct ConversionResult {
            UI* ui;
            std::string msg;
            ConversionResult(UI* u, const std::string& m) : ui(u), msg(m) {}
        };

        ConversionResult* result = new ConversionResult(ui, result_msg);

        g_idle_add([](gpointer data) -> gboolean {
            ConversionResult* result = (ConversionResult*)data;
            UI* ui = result->ui;

            if (ui->appState.conversion_in_progress) {
                ui->appState.conversion_in_progress = false;
            }

            // Hide progress bar and re-enable button
            gtk_widget_hide(ui->convertProgress);
            gtk_widget_set_sensitive(ui->convertBtn, TRUE);

            ui->appState.last_conversion_output_path = result->msg;

            delete result;

            return G_SOURCE_REMOVE;
        }, result);
    });

    conversion_thread.detach();
}

void UI::onConvertFormatChanged(GtkComboBox* combo, gpointer data) {
    // This handler can be used to update UI based on selected format
    // For now, we'll just keep it as a placeholder
}
gboolean UI::onUpdateTick(gpointer data) {      
    UI* ui = (UI*)data;      
    if (!ui->player) return TRUE;      
      
    if (ui->appState.playing) {      
        if (ui->window && gtk_widget_get_visible(ui->window)) {      
            gtk_widget_queue_draw(ui->drawingArea);      
        }      
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
        oss << ui->appState.current_track_name << " ("       
            << std::setfill('0') << std::setw(2) << cM << ":" << std::setw(2) << cS       
            << " / "       
            << std::setw(2) << dM << ":" << std::setw(2) << dS << ")";      
        gtk_label_set_text(GTK_LABEL(ui->lblInfo), oss.str().c_str());      
    } else if (!ui->appState.playlist.empty() && ui->appState.current_track_idx != -1) {      
        gtk_label_set_text(GTK_LABEL(ui->lblInfo), ui->appState.current_track_name.c_str());      
    } else {      
        gtk_label_set_text(GTK_LABEL(ui->lblInfo), "Ready");      
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
      
void UI::buildWidgets() {      
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);      
    gtk_window_set_title(GTK_WINDOW(window), "TermAMP");      
    gtk_window_set_default_size(GTK_WINDOW(window), FULL_WIDTH, FULL_HEIGHT_INIT);      
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);       
          
    Utils::setWindowIcon(window);      
      
    GtkStyleContext *context = gtk_widget_get_style_context(window);      
    gtk_style_context_add_class(context, "tm-window");      
      
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);      
    g_signal_connect(window, "key-press-event", G_CALLBACK(onKeyPress), this);      
      
    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);      
    gtk_container_set_border_width(GTK_CONTAINER(mainBox), 5);      
    gtk_container_add(GTK_CONTAINER(window), mainBox);      
      
    // --- MENU BAR ---
    GtkWidget* menuBar = gtk_menu_bar_new();
    gtk_widget_set_name(menuBar, "main-menubar");

    GtkStyleContext *menubar_context = gtk_widget_get_style_context(menuBar);
    gtk_style_context_add_class(menubar_context, GTK_STYLE_CLASS_BACKGROUND);

    // Help Menu
    GtkWidget* helpMenu = gtk_menu_new();
    GtkWidget* helpItem = gtk_menu_item_new();
    GtkWidget* helpLabel = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(helpLabel),
                 "<span foreground='#000000'>Help</span>");

    gtk_container_add(GTK_CONTAINER(helpItem), helpLabel);
    gtk_widget_show(helpLabel);

    // ✅ FIX 1: Connect submenu
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(helpItem), helpMenu);

    // About Item
    GtkWidget* aboutItem = gtk_menu_item_new();
    GtkWidget* aboutLabel = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(aboutLabel),
                 "<span foreground='#000000'>About...</span>");

    gtk_container_add(GTK_CONTAINER(aboutItem), aboutLabel);
    gtk_widget_show(aboutLabel);
    gtk_widget_show(aboutItem);

    // ✅ FIX 2: Connect callback
    g_signal_connect(aboutItem, "activate", G_CALLBACK(onAboutClicked), this);

    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), aboutItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), helpItem);

    gtk_widget_show_all(menuBar);

    // Pack Menu Bar at the TOP
    gtk_box_pack_start(GTK_BOX(mainBox), menuBar, FALSE, FALSE, 0);

    // --- END MENU BAR ---
      
    visualizerContainerBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);      
    gtk_box_pack_start(GTK_BOX(mainBox), visualizerContainerBox, FALSE, FALSE, 0);

    drawingArea = gtk_drawing_area_new();
    g_object_ref(drawingArea);
    gtk_widget_set_size_request(drawingArea, -1, VISUALIZER_FULL_HEIGHT);
    gtk_box_pack_start(GTK_BOX(visualizerContainerBox), drawingArea, FALSE, FALSE, 0);

    // Album Art Display
    albumArtImage = gtk_image_new();
    gtk_widget_set_size_request(albumArtImage, 150, 150);
    gtk_widget_set_halign(albumArtImage, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(mainBox), albumArtImage, FALSE, FALSE, 2);
    gtk_widget_hide(albumArtImage); // Initially hidden until album art is found

    lblInfo = gtk_label_new("Ready");
    gtk_label_set_ellipsize(GTK_LABEL(lblInfo), PANGO_ELLIPSIZE_END);
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

    // Equalizer Controls
    GtkWidget* eqBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

    // Equalizer Toggle and Presets
    GtkWidget* eqControlsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    eqToggle = gtk_toggle_button_new_with_label("EQ");
    eqPresetCombo = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(eqPresetCombo), NULL, "Presets");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(eqPresetCombo), NULL, "Rock");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(eqPresetCombo), NULL, "Pop");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(eqPresetCombo), NULL, "Classical");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(eqPresetCombo), NULL, "Jazz");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(eqPresetCombo), NULL, "Bass");
    gtk_combo_box_set_active(GTK_COMBO_BOX(eqPresetCombo), 0);
    GtkWidget* eqResetBtn = gtk_button_new_with_label("Reset");

    gtk_box_pack_start(GTK_BOX(eqControlsBox), eqToggle, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(eqControlsBox), eqPresetCombo, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(eqControlsBox), eqResetBtn, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(eqBox), eqControlsBox, FALSE, FALSE, 0);

    // Equalizer Bands
    eqBandsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
    eqSliders.clear();

    // Create 10 equalizer sliders for frequency bands
    for (int i = 0; i < 10; ++i) {
        GtkWidget* bandBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
        GtkWidget* label = gtk_label_new(std::to_string(i + 1).c_str());
        GtkWidget* slider = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, -24.0, 12.0, 1.0);

        gtk_scale_set_draw_value(GTK_SCALE(slider), TRUE);
        gtk_scale_set_value_pos(GTK_SCALE(slider), GTK_POS_BOTTOM);
        gtk_range_set_value(GTK_RANGE(slider), 0.0); // Default flat
        gtk_widget_set_size_request(slider, -1, 80); // Fixed height for better layout

        gtk_box_pack_start(GTK_BOX(bandBox), slider, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(bandBox), label, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(eqBandsBox), bandBox, TRUE, TRUE, 0);
        eqSliders.push_back(slider);
    }

    gtk_box_pack_start(GTK_BOX(eqBox), eqBandsBox, FALSE, FALSE, 0);
    gtk_widget_hide(eqBandsBox); // Initially hidden

    gtk_box_pack_start(GTK_BOX(mainBox), eqBox, FALSE, FALSE, 2);

    // Crossfading Controls
    GtkWidget* crossfadeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    crossfadeToggle = gtk_toggle_button_new_with_label("X-Fade");
    crossfadeDurationScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1.0, 10.0, 0.1);
    gtk_scale_set_draw_value(GTK_SCALE(crossfadeDurationScale), FALSE); // We'll use our own label
    gtk_range_set_value(GTK_RANGE(crossfadeDurationScale), appState.crossfade_duration);
    crossfadeDurationLabel = gtk_label_new(NULL);
    char initialText[20];
    sprintf(initialText, "%.1f s", appState.crossfade_duration);
    gtk_label_set_text(GTK_LABEL(crossfadeDurationLabel), initialText);

    gtk_box_pack_start(GTK_BOX(crossfadeBox), crossfadeToggle, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(crossfadeBox), crossfadeDurationScale, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(crossfadeBox), crossfadeDurationLabel, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(mainBox), crossfadeBox, FALSE, FALSE, 2);
    gtk_widget_set_sensitive(crossfadeDurationScale, false); // Initially disabled

    // Audio Conversion Controls
    GtkWidget* convertBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    convertBtn = gtk_button_new_with_label("Convert");
    convertFormatCombo = gtk_combo_box_text_new();

    // Add supported formats to the combo box
    auto formats = Utils::getSupportedOutputFormats();
    for (const auto& format : formats) {
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(convertFormatCombo), NULL, format.c_str());
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(convertFormatCombo), 0); // Default to first format
    convertProgress = gtk_progress_bar_new();
    gtk_widget_set_hexpand(convertProgress, TRUE);
    gtk_widget_hide(convertProgress); // Initially hidden

    gtk_box_pack_start(GTK_BOX(convertBox), convertBtn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(convertBox), convertFormatCombo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(convertBox), convertProgress, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(mainBox), convertBox, FALSE, FALSE, 2);

    GtkWidget* controlsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget* btnPrev = gtk_button_new_with_label("<");
    GtkWidget* btnPlay = gtk_button_new_with_label("|>");
    GtkWidget* btnPause = gtk_button_new_with_label("||");
    GtkWidget* btnStop = gtk_button_new_with_label("[]");
    GtkWidget* btnNext = gtk_button_new_with_label(">");
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
      
    GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);      
    gtk_widget_set_vexpand(scrolled, TRUE);       
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled), 150);       
    playlistBox = gtk_list_box_new();      
    g_object_ref(playlistBox);       
    gtk_container_add(GTK_CONTAINER(scrolled), playlistBox);      
    gtk_box_pack_start(GTK_BOX(mainBox), scrolled, TRUE, TRUE, 0);      
      
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
    g_signal_connect(drawingArea, "draw", G_CALLBACK(Visualizer::onDraw), visualizer);

    // Equalizer Signal Connections
    g_signal_connect(eqToggle, "toggled", G_CALLBACK(onEqToggled), this);
    g_signal_connect(eqPresetCombo, "changed", G_CALLBACK(onEqPresetChanged), this);
    g_signal_connect(eqResetBtn, "clicked", G_CALLBACK(onEqResetClicked), this);

    // Connect band sliders
    for (auto slider : eqSliders) {
        g_signal_connect(slider, "value-changed", G_CALLBACK(onEqBandChanged), this);
    }

    // Crossfading Signal Connections
    g_signal_connect(crossfadeToggle, "toggled", G_CALLBACK(onCrossfadeToggled), this);
    g_signal_connect(crossfadeDurationScale, "value-changed", G_CALLBACK(onCrossfadeDurationChanged), this);

    // Audio Conversion Signal Connections
    g_signal_connect(convertBtn, "clicked", G_CALLBACK(onConvertAudioClicked), this);
    g_signal_connect(convertFormatCombo, "changed", G_CALLBACK(onConvertFormatChanged), this);
}
      
int UI::run() {
    player = new Player(&appState);
    visualizer = new Visualizer(&appState);

    // Set up album art callback
    appState.onAlbumArtChanged = [](void* userData) {
        UI* ui = (UI*)userData;
        // Update album art from main thread
        g_idle_add([](gpointer data) -> gboolean {
            UI* ui = (UI*)data;
            ui->updateAlbumArt();
            return G_SOURCE_REMOVE;
        }, ui);
    };
    appState.albumArtCallbackUserData = this;

    buildWidgets();
    playlistMgr = new PlaylistManager(&appState, player, playlistBox);
    player->setEOSCallback([](void* data){ ((PlaylistManager*)data)->autoAdvance(); }, playlistMgr);
    g_signal_connect(playlistBox, "row-activated", G_CALLBACK(+[](GtkListBox* b, GtkListBoxRow* r, gpointer d){
        ((PlaylistManager*)d)->onRowActivated(b, r);
    }), playlistMgr);
    g_timeout_add(100, onUpdateTick, this);
    gtk_widget_show_all(window);

    // Initialize equalizer bands to the current values in app state
    if (appState.eq_bands.size() == 10) {
        player->setEqualizerBands(appState.eq_bands);
    } else {
        // Default to flat response if not properly initialized
        std::vector<double> flatBands(10, 0.0);
        player->setEqualizerBands(flatBands);
    }

    // Initialize crossfading toggle state
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(crossfadeToggle), appState.crossfading_enabled);
    gtk_widget_set_sensitive(crossfadeDurationScale, appState.crossfading_enabled);

    gtk_main();
    return 0;
}
