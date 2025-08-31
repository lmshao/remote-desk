# remote-desk
A learning project for building a remote desktop tool with RTSP/RTMP/...

## Dependencies

### Windows Dependencies

#### FFmpeg (Required for video encoding)
**Option 1: PowerShell Direct Download**
```powershell
.\scripts\setup_ffmpeg_windows.ps1
```

**Option 2: Manual Setup**
1. Download FFmpeg development libraries from: https://www.gyan.dev/ffmpeg/builds/
2. Download `ffmpeg-X.X-full_build-shared.7z`
3. Extract and copy directories:
   - `include/` → `remote-desk/ffmpeg/include/`
   - `lib/` → `remote-desk/ffmpeg/lib/`
   - `bin/` → `remote-desk/ffmpeg/dll/`

#### Visual Studio Build Tools
Install Visual Studio 2019/2022 with C++ development tools, or download "Build Tools for Visual Studio".

### Linux Dependencies

#### X11 Libraries (Required for screen capture)
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

#### FFmpeg (Required for video encoding)
```bash
# Ubuntu/Debian
sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev

# CentOS/RHEL/Fedora
sudo yum install ffmpeg-devel
# or for newer versions:
sudo dnf install ffmpeg-devel
```

### Quick Install (All Dependencies)
Install all required dependencies at once:

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential cmake pkg-config libx11-dev libxext-dev libavcodec-dev libavformat-dev libavutil-dev libswscale-dev

# CentOS/RHEL/Fedora
sudo dnf install gcc-c++ cmake pkgconfig libX11-devel libXext-devel ffmpeg-devel
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

### Windows
```cmd
# Set up FFmpeg dependencies first (if not done already)
scripts\setup_ffmpeg_windows.bat

# Build the project
mkdir build && cd build
cmake ..
cmake --build .
```

### Linux
**Prerequisites**: Make sure you have installed all the required development libraries mentioned in the Dependencies section above.

```bash
mkdir build && cd build
cmake ..
make
```

**Note**: 
- On Linux: X11 and FFmpeg libraries are required for compilation
- On Windows: FFmpeg libraries will be automatically detected in the ffmpeg/ directory
- If you encounter compilation errors, ensure all development packages are properly installed

## Running Examples

```bash
# Make sure X11 libraries are installed and display is available
cd build/examples
./screen_capturer_example
```

**Note**: If running in a headless environment, make sure to start Xvfb and set the DISPLAY environment variable as shown above.
