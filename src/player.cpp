#include "player.h"
#include <iostream>
#include <filesystem> // C++17 feature for filename extraction

Player::Player(AppState* state) : app(state) {
    gst_init(NULL, NULL);
    pipeline = gst_element_factory_make("playbin", "player");
    
    if (!pipeline) {
        std::cerr << "CRITICAL: Failed to create GStreamer playbin." << std::endl;
        return;
    }

    GstBus* bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, busCallback, this);
    gst_object_unref(bus);
}

Player::~Player() {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }
}

void Player::setEOSCallback(EOSCallback cb, void* data) {
    onEOS = cb;
    eosData = data;
}

void Player::load(const std::string& path) {
    stop(); 
    
    // 1. Set Fallback Name (Filename) immediately
    // This ensures we don't show the OLD song title while loading the NEW one
    std::string filename = std::filesystem::path(path).filename().string();
    app->current_track_name = filename; // Default to filename
    
    // 2. Load GStreamer URI
    GError *error = NULL;
    gchar *uri = gst_filename_to_uri(path.c_str(), &error);
    
    if (error) {
        if (path.find("file://") == 0 || path.find("http://") == 0) {
             g_object_set(G_OBJECT(pipeline), "uri", path.c_str(), NULL);
        } else {
             std::cerr << "URI Error: " << error->message << std::endl;
        }
        g_error_free(error);
    } else {
        g_object_set(G_OBJECT(pipeline), "uri", uri, NULL);
        g_free(uri);
    }
}

void Player::play() {
    if (!pipeline) return;
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    app->playing = true;
    app->paused = false;
}

void Player::pause() {
    if (!pipeline) return;
    gst_element_set_state(pipeline, GST_STATE_PAUSED);
    app->playing = false;
    app->paused = true;
}

void Player::stop() {
    if (!pipeline) return;
    gst_element_set_state(pipeline, GST_STATE_NULL);
    app->playing = false;
    app->paused = false;
    app->current_track_name = "Ready"; // Reset on full stop
}

void Player::setVolume(double volume) {
    if (!pipeline) return;
    g_object_set(G_OBJECT(pipeline), "volume", volume, NULL);
}

void Player::seek(double seconds) {
    if (!pipeline) return;
    gst_element_seek_simple(pipeline, GST_FORMAT_TIME, 
        (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), 
        (gint64)(seconds * GST_SECOND));
}

double Player::getPosition() {
    if (!pipeline) return 0.0;
    gint64 pos = 0;
    if (gst_element_query_position(pipeline, GST_FORMAT_TIME, &pos)) {
        return (double)pos / GST_SECOND;
    }
    return 0.0;
}

double Player::getDuration() {
    if (!pipeline) return 0.0;
    gint64 dur = 0;
    if (gst_element_query_duration(pipeline, GST_FORMAT_TIME, &dur)) {
        return (double)dur / GST_SECOND;
    }
    return 0.0;
}

// --- NEW: Tag Handler Helper ---
void Player::handleTags(GstTagList* tags) {
    gchar *artist = NULL;
    gchar *title = NULL;

    // Try to get Artist and Title
    gst_tag_list_get_string(tags, GST_TAG_ARTIST, &artist);
    gst_tag_list_get_string(tags, GST_TAG_TITLE, &title);

    if (title) {
        std::string meta;
        if (artist) {
            meta = std::string(artist) + " - " + std::string(title);
        } else {
            meta = std::string(title);
        }
        
        // Update App State
        app->current_track_name = meta;
        
        g_free(title);
        if (artist) g_free(artist);
    }
}

gboolean Player::busCallback(GstBus* bus, GstMessage* msg, gpointer data) {
    Player* player = (Player*)data;
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            if (player->onEOS) player->onEOS(player->eosData);
            else player->stop();
            break;
        case GST_MESSAGE_ERROR:
            player->stop();
            break;
        // --- NEW: Handle Metadata Tags ---
        case GST_MESSAGE_TAG: {
            GstTagList *tags = NULL;
            gst_message_parse_tag(msg, &tags);
            player->handleTags(tags);
            gst_tag_list_unref(tags);
            break;
        }
        default:
            break;
    }
    return TRUE;
}
