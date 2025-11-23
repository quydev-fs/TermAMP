# Building TermuxMusic95

This guide covers installation of dependencies and building TermuxMusic95 from source.

***

## 📋 Prerequisites

### Termux (Android)

```sh
# Update packages
pkg update && pkg upgrade

# Install required repositories
pkg install clang make x11-repo

# Install dependencies
pkg install termux-x11-nightly pkg-config gtk3 gstreamer gst-plugins-base gst-plugins-good
```

### Debian/Ubuntu-based Linux

```sh
# Update package list
sudo apt update

# Install build tools
sudo apt install build-essential clang pkg-config libstdc++-dev

# Install GTK3 and GStreamer
sudo apt install libgtk-3-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-plugins-good
```

### Arch Linux

```sh
# Install development tools
sudo pacman -S base-devel clang

# Install GTK3 and GStreamer libraries
sudo pacman -S gtk3 gstreamer gst-plugins-base gst-plugins-good
```

### Fedora/RHEL-based Linux

```sh
# Install development tools
sudo dnf groupinstall "Development Tools"
sudo dnf install clang

# Install GTK3 and GStreamer
sudo dnf install gtk3-devel gstreamer1-devel gstreamer1-plugins-base-devel gstreamer1-plugins-good
```

***

## 🖥️ X11 Setup (Termux Only)

TermuxMusic95 requires an X11 server to display its GUI on Android.

### 1. Install Termux:X11 app

Download from [GitHub Releases](https://github.com/termux/termux-x11/releases) or F-Droid.

### 2. Start X11 server

In Termux, run:
```sh
termux-x11 :0 &
```

### 3. Set DISPLAY variable

```sh
export DISPLAY=:0
```

**Tip:** Add this to your `~/.bashrc` or `~/.zshrc`:
```sh
echo 'export DISPLAY=:0' >> ~/.bashrc
source ~/.bashrc
```

***

## 🏗️ Building from Source

### 1. Clone the repository

```sh
git clone https://github.com/quydev-fs/TermuxMusic95.git
cd TermuxMusic95
```

### 2. Build the project

```sh
make
```

This will:
- Compile all source files in `src/`
- Generate object files in `build/obj/`
- Create the executable at `build/bin/TermuxMusic95`

### 3. Run the player

```sh
./build/bin/TermuxMusic95
```

***

## 🔧 Build Options

### Clean build

Remove all build artifacts:
```sh
make clean
```

### Rebuild from scratch

```sh
make clean
make
```

### Install system-wide (optional)

```sh
ln -s $HOME/TermuxMusic95/build/bin/TermuxMusic95 $PREFIX/bin/TermuxMusic95
```

Now you can run `TermuxMusic95` from anywhere!

***

## 🐛 Troubleshooting

### "Package gtk+-3.0 was not found"

**Solution:** Install GTK3 development files
```sh
# Termux
pkg install gtk3

# Debian/Ubuntu
sudo apt install libgtk-3-dev

# Arch
sudo pacman -S gtk3
```

### "Package gstreamer-1.0 was not found"

**Solution:** Install GStreamer development files
```sh
# Termux
pkg install gstreamer gst-plugins-base

# Debian/Ubuntu
sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev

# Arch
sudo pacman -S gstreamer gst-plugins-base
```

### X11 connection errors (Termux)

**Symptoms:** "Cannot open display :0"

**Solution:**
1. Make sure Termux:X11 app is running
2. Start X server: `termux-x11 :0 &`
3. Set DISPLAY: `export DISPLAY=:0`
4. Check if X11 is working: `xeyes` (if installed)

### Audio not playing

**Solution:** Install GStreamer audio plugins
```sh
# Termux
pkg install gst-plugins-good

# Debian/Ubuntu
sudo apt install gstreamer1.0-plugins-good gstreamer1.0-pulseaudio

# Arch
sudo pacman -S gst-plugins-good
```

***

## 📊 Project Structure

```
TermuxMusic95/
├── src/              # C++ source files
│   ├── main.cpp
│   ├── player.cpp
│   ├── playlist.cpp
│   ├── ui.cpp
│   └── visualizer.cpp
├── include/          # Header files
│   ├── common.h
│   ├── player.h
│   ├── playlist.h
│   ├── ui.h
│   └── visualizer.h
├── build/            # Build artifacts (generated)
│   ├── obj/         # Object files
│   └── bin/         # Executable
├── assets/           # Resources
│   └── icons/
├── Makefile          # Build configuration
└── README.md
```

***

## 🚀 Quick Start Script

For convenience, here's a one-liner to build and run:

```sh
git clone https://github.com/quydev-fs/TermuxMusic95.git && cd TermuxMusic95 && make && ./build/bin/TermuxMusic95
```

***

## 🔗 Related Documentation

- [README.md](README.md) - Project overview and usage
- [LICENSE](LICENSE) - MIT License
- [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution guidelines

***

**Need help?** Open an issue on [GitHub](https://github.com/quydev-fs/TermuxMusic95/issues)!
