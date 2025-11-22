# TermuxMusic95

<p align="center">
  <img src="assets/icons/logo.jpg" />
</p>

*An X11-based MP3 player for Termux, inspired by WinAMP 2.x and written entirely in C++.*

***

## 🚀 Overview

**TermuxMusic95** brings the nostalgic look and functionality of WinAMP 2.x to Android’s Termux environment. With a familiar GUI rendered via X11, robust MP3 playback thanks to `mpg123`, PulseAudio integration, and real-time spectrum visualizations via FFT, this project demonstrates what’s possible with modern terminal environments on Android.

***

## ✨ Features

- Classic WinAMP-inspired graphical user interface (X11)
- MP3 playback with metadata support (`mpg123`)
- PulseAudio streaming
- Real-time spectrum analyzer (FFT-based)
- Keyboard and mouse controls
- Playlist and folder support

***

## 🛠️ Prerequisites

Make sure you have these components installed in your Termux environment:

```sh
pkg install clang make x11-repo
pkg install termux-x11 pulseaudio xproto libmpg123
```

1. **X Server:**  
   Launch an X server with:
   ```sh
   termux-x11 :0 &
   ```
2. **Set the DISPLAY variable:**
   ```sh
   export DISPLAY=:0
   ```

***

## 📂 Project Structure
| Folder / File | Description                                         |
| ------------- | --------------------------------------------------- |
| src/          | C++ source code files                               |
| include/      | C++ header files                                    |
| build/        | Compiled build artifacts (executables, binaries)    |
| Makefile      | Build script usingg++, links all required libraries |
| main.cpp      | Entry point, parses arguments and playlist          |

***

## 🏗️ Build Instructions

1. **Clone the repository and change into its directory:**
   ```sh
   git clone https://github.com/quydev-fs/TermuxMusic95.git
   cd TermuxMusic95
   ```
2. **Build the project:**
   ```sh
   make
   ```
   This will generate the `TermuxMusic95` executable.

***

## ▶️ Usage

The player takes a file, directory, or `.m3u` playlist as a command-line argument:

```sh
# Start TermuxMusic95 without loading anything
./build/bin/TermuxMusic95
# Play a single file
./build/bin/TermuxMusic95 /sdcard/Music/track.mp3

# Play all MP3s in a directory
./build/bin/TermuxMusic95 /sdcard/Music/AlbumFolder

# Play using an M3U playlist
./build/bin/TermuxMusic95 /sdcard/Playlists/playlist.m3u
```

***

## ⌨️ Controls

**Mouse:**  
- Click buttons, seek bar, and drag the volume slider

**Keyboard shortcuts:**

| Key        | Action             |
|------------|--------------------|
| Z          | Previous Track     |
| X          | Play/Resume        |
| C          | Pause              |
| V          | Stop/Reset         |
| B          | Next Track         |
| Up Arrow   | Volume Up          |
| Down Arrow | Volume Down        |

***

## 📄 License

This project is licensed under the [MIT License](LICENSE).

***

## 🙋 FAQ & Contribution

- **Bugs and feature requests:** Please open issues and PRs on GitHub.
- **Contact:** See repository issues or discussions for support

*"it may better when it comes to retro"*
