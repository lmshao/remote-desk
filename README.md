# remote-desk
A learning project for building a remote desktop tool with RTSP/RTMP/...

## Dependencies

### Linux X11 Libraries
For screen capture functionality on Linux, you need to install X11 development libraries:

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install libx11-dev libxext-dev

# CentOS/RHEL/Fedora
sudo yum install libX11-devel libXext-devel
# or for newer versions:
sudo dnf install libX11-devel libXext-devel
```

### Quick Install (All Dependencies)
Install all required dependencies at once:

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential cmake pkg-config libx11-dev libxext-dev

# CentOS/RHEL/Fedora
sudo dnf install gcc-c++ cmake pkgconfig libX11-devel libXext-devel
```

### Virtual Display (Headless Environment)
If you're running in a headless environment (no physical display), you need to install and configure a virtual display server:

```bash
# Install Xvfb (X Virtual Framebuffer)
sudo apt-get install xvfb  # Ubuntu/Debian
sudo yum install xorg-x11-server-Xvfb  # CentOS/RHEL
sudo dnf install xorg-x11-server-Xvfb  # Fedora

# Start virtual display
Xvfb :99 -screen 0 1024x768x24 &

# Set DISPLAY environment variable
export DISPLAY=:99

# Now you can run screen capture applications
./screen_capturer_example
```

## Build Instructions

**Prerequisites**: Make sure you have installed all the required development libraries mentioned in the Dependencies section above.

```bash
mkdir build && cd build
cmake ..
make
```

**Note**: 
- X11 libraries are required for compilation
- If you encounter compilation errors, ensure all development packages are properly installed
- For Ubuntu/Debian systems, you may also need: `sudo apt-get install build-essential cmake pkg-config`

## Running Examples

```bash
# Make sure X11 libraries are installed and display is available
cd build/examples
./screen_capturer_example
```

**Note**: If running in a headless environment, make sure to start Xvfb and set the DISPLAY environment variable as shown above.
