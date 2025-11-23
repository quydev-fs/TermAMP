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
    stop(); // Reset pipeline state
    
    // FIX: Use GStreamer's built-in URI converter
    // This handles spaces, special chars, and absolute paths automatically.
    GError *error = NULL;
    gchar *uri = gst_filename_to_uri(path.c_str(), &error);
    
    if (error) {
        // If conversion failed, maybe it's already a URI (http://... or file://...)
        if (path.find("file://") == 0 || path.find("http://") == 0) {
             g_object_set(G_OBJECT(pipeline), "uri", path.c_str(), NULL);
        } else {
             std::cerr << "URI Error: " << error->message << " [" << path << "]" << std::endl;
        }
        g_error_free(error);
    } else {
        // std::cout << "Loading URI: " << uri << std::endl; // Debug
        g_object_set(G_OBJECT(pipeline), "uri", uri, NULL);
        g_free(uri);
    }
}

void Player::play() {
    if (!pipeline) return;
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Error: Failed to start playback." << std::endl;
    } else {
        app->playing = true;
        app->paused = false;
    }
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

gboolean Player::busCallback(GstBus* bus, GstMessage* msg, gpointer data) {
    Player* player = (Player*)data;
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            // In a full implementation, trigger 'next track' here
            player->stop();
            break;
        case GST_MESSAGE_ERROR: {
            GError *err;
            gchar *debug;
            gst_message_parse_error(msg, &err, &debug);
            std::cerr << "GStreamer Error: " << err->message << std::endl;
            if (debug) {
                // std::cerr << "Debug info: " << debug << std::endl;
                g_free(debug);
            }
            g_error_free(err);
            player->stop();
            break;
        }
        default:
            break;
    }
    return TRUE;
}
