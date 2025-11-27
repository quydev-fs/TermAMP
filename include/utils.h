#ifndef UTILS_H
#define UTILS_H

#include <gtk/gtk.h>
#include <string>
#include <vector>

class Utils {
public:
    // Calculates path relative to the executable
    static std::string getResourcePath(const std::string& assetName);

    // Loads the CSS file into the global screen provider
    static void loadGlobalCSS();

    // Sets the window icon
    static void setWindowIcon(GtkWidget* window);

    // Helper to get an image widget for the About dialog
    static GtkWidget* createLogoImage(int size);

    // Audio format conversion functions
    static bool convertAudioFormat(const std::string& inputPath, const std::string& outputPath, const std::string& format);
    static std::vector<std::string> getSupportedOutputFormats();
    static bool isFormatSupported(const std::string& format);

    // Progress callback for conversion
    typedef void (*ConversionProgressCallback)(double progress, void* userData);
    static void setProgressCallback(ConversionProgressCallback callback, void* userData);

    // Album art functions
    static GdkPixbuf* extractAlbumArt(const std::string& audioFilePath, int size = 100);
    static GdkPixbuf* loadAlbumArtFromCache(const std::string& audioFilePath, int size = 100);
    static void cacheAlbumArt(const std::string& audioFilePath, GdkPixbuf* pixbuf);

private:
    static ConversionProgressCallback progressCallback;
    static void* callbackUserData;
};

#endif
