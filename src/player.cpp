#include "player.h"
#include <iostream>
#include <filesystem>

Player::Player(AppState* state) : app(state), last_play_time(0), equalizer(nullptr), next_pipeline(nullptr), crossfade_timeout_id(0) {
    gst_init(NULL, NULL);
    pipeline = gst_element_factory_make("playbin", "player");

    if (!pipeline) {
        std::cerr << "CRITICAL: Failed to create GStreamer playbin." << std::endl;
        return;
    }

    // Initialize equalizer bands (10-band equalizer with default values of 0.0)
    app->eq_bands = std::vector<double>(10, 0.0);

    GstBus* bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, busCallback, this);
    gst_object_unref(bus);
}

Player::~Player() {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }

    if (equalizer) {
        gst_element_set_state(equalizer, GST_STATE_NULL);
        gst_object_unref(equalizer);
    }

    if (next_pipeline) {
        gst_element_set_state(next_pipeline, GST_STATE_NULL);
        gst_object_unref(next_pipeline);
    }

    if (crossfade_timeout_id > 0) {
        g_source_remove(crossfade_timeout_id);
    }
}

void Player::setEOSCallback(EOSCallback cb, void* data) {
    onEOS = cb;
    eosData = data;
}

GstState Player::getState() {
    GstState state = GST_STATE_NULL;
    if (pipeline) gst_element_get_state(pipeline, &state, NULL, 100);
    return state;
}

void Player::load(const std::string& path) {
    stop();
    std::string filename = std::filesystem::path(path).filename().string();
    app->current_track_name = filename;

    GError *error = NULL;
    gchar *uri = gst_filename_to_uri(path.c_str(), &error);

    if (error) {
        if (path.find("file://") == 0 || path.find("http://") == 0) {
             g_object_set(G_OBJECT(pipeline), "uri", path.c_str(), NULL);
        }
        g_error_free(error);
    } else {
        g_object_set(G_OBJECT(pipeline), "uri", uri, NULL);
        g_free(uri);
    }
}

void Player::play() {
    if (!pipeline) return;

    // CRITICAL: Record time.
    last_play_time = g_get_monotonic_time();

    // Ensure the equalizer is set up before playing
    setupEqualizer();

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
    app->current_track_name = "Ready"; 
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

void Player::handleTags(GstTagList* tags) {
    gchar *artist = NULL;
    gchar *title = NULL;
    gst_tag_list_get_string(tags, GST_TAG_ARTIST, &artist);
    gst_tag_list_get_string(tags, GST_TAG_TITLE, &title);

    if (title) {
        std::string meta;
        if (artist) meta = std::string(artist) + " - " + std::string(title);
        else meta = std::string(title);
        app->current_track_name = meta;
        g_free(title);
        if (artist) g_free(artist);
    }
}

void Player::setupEqualizer() {
    if (!pipeline) return;

    // Create the equalizer element (10-band)
    if (!equalizer) {
        equalizer = gst_element_factory_make("equalizer-10bands", "equalizer");
        if (!equalizer) {
            std::cerr << "Failed to create equalizer element. Ensure GStreamer audio plugin is installed." << std::endl;
            return;
        }

        // Set the equalizer bands to the stored values
        setEqualizerBands(app->eq_bands);

        // Add the equalizer as an audio filter to the playbin
        g_object_set(G_OBJECT(pipeline), "audio-filter", equalizer, NULL);
    }

    // Apply the enabled state
    enableEqualizer(app->eq_enabled);
}

void Player::setEqualizerBand(int band, double value) {
    if (!equalizer || band < 0 || band >= 10) return;

    // Clamp the value to reasonable range (-24.0 to +12.0 dB)
    double clampedValue = std::max(-24.0, std::min(12.0, value));

    // Update the application state
    if (band < app->eq_bands.size()) {
        app->eq_bands[band] = clampedValue;
    } else {
        // Extend the vector if needed
        app->eq_bands.resize(band + 1, 0.0);
        app->eq_bands[band] = clampedValue;
    }

    // Construct the property name for the band (band0, band1, etc.)
    std::string bandProperty = "band" + std::to_string(band);
    g_object_set(G_OBJECT(equalizer), bandProperty.c_str(), clampedValue, NULL);
}

void Player::setEqualizerBands(const std::vector<double>& bands) {
    for (size_t i = 0; i < bands.size(); ++i) {
        setEqualizerBand(i, bands[i]);
    }
}

void Player::enableEqualizer(bool enabled) {
    if (!equalizer) return;

    app->eq_enabled = enabled;

    // Set the "enabled" property if the equalizer supports it
    // Note: not all equalizer implementations have an 'enabled' property
    // If not available, we'll just set all bands to 0 when disabled
    if (enabled) {
        setEqualizerBands(app->eq_bands);
    } else {
        // When disabled, set all bands to 0 to effectively disable the effect
        std::vector<double> zeroBands(app->eq_bands.size(), 0.0);
        setEqualizerBands(zeroBands);
    }
}

bool Player::isEqualizerEnabled() const {
    return app->eq_enabled;
}

// Crossfading implementation
gboolean Player::updateCrossfade() {
    if (!app->crossfading_enabled) {
        stopCrossfade();
        return FALSE; // Stop the timer
    }

    double currentPos = getPosition();
    double duration = getDuration();
    double remaining = duration - currentPos;

    if (remaining <= app->crossfade_duration) {
        // Start the next track if it's not already playing
        if (next_pipeline && getState() == GST_STATE_PLAYING) {
            gst_element_set_state(next_pipeline, GST_STATE_PLAYING);

            // Fade out current track
            double fadeOutVolume = (remaining / app->crossfade_duration) * app->volume;
            setVolume(fadeOutVolume);

            // Fade in next track
            double fadeInVolume = ((app->crossfade_duration - remaining) / app->crossfade_duration) * app->volume;
            g_object_set(G_OBJECT(next_pipeline), "volume", fadeInVolume, NULL);
        }
    }

    if (remaining <= 0) {
        // Switch to the next pipeline completely
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = next_pipeline;
        next_pipeline = nullptr;
        app->is_crossfading = false;
        return FALSE; // Stop the timer
    }

    return TRUE; // Continue the timer
}

gboolean Player::crossfadeTimer(gpointer data) {
    Player* player = (Player*)data;
    return player->updateCrossfade();
}

void Player::startCrossfade(const std::string& nextUri) {
    if (!app->crossfading_enabled || nextUri.empty()) return;

    // Create the next pipeline
    next_pipeline = gst_element_factory_make("playbin", "next_player");
    if (!next_pipeline) {
        std::cerr << "Failed to create next pipeline for crossfading." << std::endl;
        return;
    }

    // Set URI for the next track
    GError *error = NULL;
    gchar *uri = gst_filename_to_uri(nextUri.c_str(), &error);
    if (error) {
        if (nextUri.find("file://") == 0 || nextUri.find("http://") == 0) {
             g_object_set(G_OBJECT(next_pipeline), "uri", nextUri.c_str(), NULL);
        }
        g_error_free(error);
    } else {
        g_object_set(G_OBJECT(next_pipeline), "uri", uri, NULL);
        g_free(uri);
    }

    // Set volume to 0 initially for fade-in effect
    g_object_set(G_OBJECT(next_pipeline), "volume", 0.0, NULL);

    // Set up the equalizer for the next pipeline if needed
    setupEqualizer();

    app->is_crossfading = true;

    // Start the crossfade timer
    if (crossfade_timeout_id > 0) {
        g_source_remove(crossfade_timeout_id);
    }
    crossfade_timeout_id = g_timeout_add(50, crossfadeTimer, this); // 50ms interval for smooth fade
}

void Player::stopCrossfade() {
    if (crossfade_timeout_id > 0) {
        g_source_remove(crossfade_timeout_id);
        crossfade_timeout_id = 0;
    }

    if (next_pipeline) {
        gst_element_set_state(next_pipeline, GST_STATE_NULL);
        gst_object_unref(next_pipeline);
        next_pipeline = nullptr;
    }

    app->is_crossfading = false;
}

bool Player::isCrossfading() const {
    return app->is_crossfading;
}

gboolean Player::busCallback(GstBus* bus, GstMessage* msg, gpointer data) {
    Player* player = (Player*)data;
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS: {
            // CRITICAL FIX: Ignore EOS if happened < 2000ms (2 sec) after play/resume command.
            // Termux/Android sometimes sends a flush EOS on resume.
            guint64 now = g_get_monotonic_time();
            if (now < player->last_play_time + 2000000) {
                std::cerr << "[PLAYER] Spurious EOS ignored (Timestamp Guard)" << std::endl;
                break;
            }

            // Check if we actually intend to be playing
            if (player->app->playing) {
                if (player->onEOS) player->onEOS(player->eosData);
                else player->stop();
            }
            break;
        }
        case GST_MESSAGE_ERROR:
            player->stop();
            break;
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
