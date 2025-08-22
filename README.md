# RemoteDesk - Remote Desktop Service

A cross-platform remote desktop streaming service that provides RTSP/RTMP streaming capabilities for desktop sharing.

## Features

- Cross-platform support (Linux, Windows, macOS)
- Desktop screen capture
- Video encoding (H.264, H.265)
- Multiple streaming protocols (RTSP, RTMP, RTP)
- Device discovery on local network
- Modular plugin architecture

## Build Requirements

- CMake 3.10 or higher
- C++17 compatible compiler (GCC, Clang, MSVC)
- Make or Ninja build system

## Quick Start

### Building the Project

#### Option 1: Using the Build Script (Recommended)

```bash
# Release build (default)
./build.sh

# Debug build
./build.sh --debug

# Clean and rebuild
./build.sh --clean --release

# Build with custom number of parallel jobs
./build.sh --jobs 8

# Show help
./build.sh --help
```

#### Option 2: Manual CMake Build

```bash
# Create build directory
mkdir build && cd build

# Configure (Release)
cmake -DCMAKE_BUILD_TYPE=Release ..

# Configure (Debug)
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Build
cmake --build . --parallel $(nproc)
```

### Running the Application

After successful build, the executable will be created in the `build` directory:

```bash
# Run from build directory
cd build
./remote-desk

# Or run directly
./build/remote-desk
```

## Build Script Options

The `build.sh` script provides several convenient options:

| Option | Description |
|--------|-------------|
| `-d, --debug` | Build in debug mode with debug symbols |
| `-r, --release` | Build in release mode with optimizations (default) |
| `-c, --clean` | Clean the build directory before building |
| `-v, --verbose` | Enable verbose build output |
| `-j, --jobs N` | Set number of parallel build jobs |
| `-h, --help` | Show help information |

## Project Structure

```
remote-desk/
├── src/                    # Source files
│   ├── core/              # Core application files
│   │   └── main.cpp       # Main entry point
│   ├── discovery/         # Device discovery module
│   │   ├── discovery.h
│   │   └── discovery.cpp
│   └── log/               # Logging utilities
│       └── remote_desk_log.h
├── build/                 # Build directory (generated)
├── CMakeLists.txt        # CMake configuration
├── build.sh              # Build script
└── README.md             # This file
```

## Build Output

- **Debug build**: Larger executable (~60KB) with debug symbols and all logging enabled
- **Release build**: Smaller executable (~20KB) with optimizations and minimal logging

## Development

### Adding New Source Files

The CMake configuration automatically discovers all `.cpp` files in the `src` directory and its subdirectories. Simply add your new files to the appropriate location and rebuild.

### IDE Integration

The build generates `compile_commands.json` for IDE integration with tools like clangd, VSCode, etc.

### Debugging

For debugging, use the debug build:

```bash
./build.sh --debug
gdb ./build/remote-desk
```

## Logging

The project includes a custom logging system with different levels:

- `LOG_DEBUG()`: Only available in debug builds
- `LOG_WARN()`: Available in debug builds
- `LOG_ERROR()`: Available in all builds

Example usage:
```cpp
LOG_DEBUG("Debug message: %s", variable);
LOG_WARN("Warning: %d", value);
LOG_ERROR("Error occurred: %s", error_string);
```

## Building on Different Platforms

### Linux
```bash
./build.sh
```

### macOS
```bash
./build.sh
```

### Windows (using MSYS2/MinGW or Visual Studio)
```bash
# Using MSYS2/MinGW
./build.sh

# Using Visual Studio (PowerShell)
mkdir build && cd build
cmake -G "Visual Studio 16 2019" ..
cmake --build . --config Release
```

## Troubleshooting

### Common Issues

1. **CMake not found**: Install CMake from https://cmake.org/
2. **Compiler not found**: Install GCC, Clang, or Visual Studio
3. **Build fails**: Try cleaning the build directory: `./build.sh --clean`

### Getting Help

For build issues, run with verbose output:
```bash
./build.sh --verbose --debug
```

This will show detailed compilation commands and help identify issues.

## License

MIT License - see LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Build and test: `./build.sh --debug`
5. Submit a pull request

## Version History

- v0.0.1: Initial project setup with basic build system