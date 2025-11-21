# TermuxMusic95: The X11 MP3 Player for Termux

TermuxMusic95 is a modular, C++ MP3 player designed specifically for the Termux environment on Android, leveraging X11 for its classic Winamp 2.x-style graphical interface. It uses `mpg123` for decoding, `PulseAudio` for sound output, and implements a Fast Fourier Transform (FFT) for real-time visualization.

## 💾 Prerequisites

Before compiling and running, you must install the necessary packages in your Termux environment:

1.  **Core Development Tools:**
    ```bash
    pkg install clang make x11-repo
    ```
2.  **Required Libraries:**
    ```bash
    pkg install termux-x11 pulseaudio xproto
    ```
3.  **Setup X11:**
    TermuxMusic95 requires an active X server. Run the following command to start it (if it's not already running):
    ```bash
    termux-x11 :0 &
    ```
4.  **Set Display Variable:**
    Ensure your terminal session knows where to send the X11 window:
    ```bash
    export DISPLAY=:0
    ```

## 🏗️ Project Structure

The player is organized into several modular files for maintainability:

| File | Description |
| :--- | :--- |
| `Makefile` | Build script using `g++` and linking all required libraries. |
| `main.cpp` | Application entry point and playlist argument parsing. |
| `AppState.h` | Defines shared data structures, application state, and global constants (like colors and dimensions). |
| `GUI.h`/`GUI.cpp` | Handles X11 window management, drawing the Winamp 2.x interface, and processing user input (mouse/keyboard). |
| `AudioEngine.h`/`AudioEngine.cpp` | Manages the playback thread, `mpg123` decoding, PulseAudio streaming, volume control, and metadata updates. |
| `FFT.h`/`FFT.cpp` | Contains the Fast Fourier Transform (FFT) logic used for the real-time spectrum analyzer visualization. |

## 🛠️ Building

1.  **Save all files:** Ensure all source files (`.cpp`, `.h`, and `Makefile`) are saved in the same directory.
2.  **Compile:** Use the provided `Makefile` to compile the project:
    ```bash
    make
    ```
    This creates an executable file named `TermuxMusic95`.

## ▶️ Running

The player requires a playlist source (MP3 file, folder, or `.m3u` file) as a command-line argument.

```bash
# Example 1: Play a single MP3 file
./TermuxMusic95 /sdcard/Music/awesome_track.mp3

# Example 2: Load all MP3 files from a directory
./TermuxMusic95 /sdcard/Music/MyAlbum

# Example 3: Load a specific playlist file
./TermuxMusic95 /sdcard/Playlists/retro.m3u
```
## ⌨️ Controls
The player supports both mouse input (clicking the virtual buttons, seeking on the bar, and dragging the volume slider) and keyboard shortcuts:
| Key | Function |
|---|---|
| Z | Previous Track |
| X | Play/Resume |
| C | Pause |
| V | Stop (Resets track position) |
| B | Next Track |
| Up Arrow | Increase Volume |
| Down Arrow | Decrease Volume |
