#include "player.h"
#include <iostream>

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

void Player::load(const std::string& path) {
    stop(); 
    
    GError *error = NULL;
    gchar *uri = gst_filename_to_uri(path.c_str(), &error);
    
    if (error) {
        // Fallback for already formatted URIs
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
}

// --- NEW: Get Position ---
double Player::getPosition() {
    if (!pipeline) return 0.0;
    gint64 pos = 0;
    if (gst_element_query_position(pipeline, GST_FORMAT_TIME, &pos)) {
        return (double)pos / GST_SECOND;
    }
    return 0.0;
}

gboolean Player::busCallback(GstBus* bus, GstMessage* msg, gpointer data) {
    Player* player = (Player*)data;
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            // TODO: Auto-advance logic should be triggered here in a full app
            player->stop();
            break;
        case GST_MESSAGE_ERROR:
            player->stop();
            break;
        default:
            break;
    }
    return TRUE;
}
