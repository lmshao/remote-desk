cmake_minimum_required(VERSION 3.10)
project(remote-desk VERSION 0.0.1 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(BUILD_STATIC_LIBS "Build static libraries" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_PLUGINS "Build service plugins as separate .so files" OFF)

# Set default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Display configuration options for developers
message(STATUS "")
message(STATUS "========== Remote Desk Library Configuration Options ==========")
message(STATUS "")
message(STATUS "Build Type Configuration:")
message(STATUS "  Current build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Available types: Debug, Release, RelWithDebInfo, MinSizeRel")
message(STATUS "  Change with: cmake -DCMAKE_BUILD_TYPE=Debug ..")
message(STATUS "")
message(STATUS "Library Build Options:")
message(STATUS "  BUILD_STATIC_LIBS: Build static libraries (current: ${BUILD_STATIC_LIBS})")
message(STATUS "  BUILD_SHARED_LIBS: Build shared libraries (current: ${BUILD_SHARED_LIBS})")
message(STATUS "  BUILD_EXAMPLES: Build example programs (current: ${BUILD_EXAMPLES})")
message(STATUS "  BUILD_TESTS: Build unit tests (current: ${BUILD_TESTS})")
message(STATUS "  BUILD_PLUGINS: Build service plugins (current: ${BUILD_PLUGINS})")
message(STATUS "")
message(STATUS "Usage Examples:")
message(STATUS "  Debug build:     cmake -DCMAKE_BUILD_TYPE=Debug ..")
message(STATUS "  Release build:   cmake -DCMAKE_BUILD_TYPE=Release ..")
message(STATUS "  Minimal build:   cmake -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF ..")
message(STATUS "  Plugin mode:     cmake -DBUILD_PLUGINS=ON ..")
message(STATUS "")
message(STATUS "Note: Dependencies (coreutils/network) automatically follow main project build type")
message(STATUS "")
message(STATUS "=================================================================")

# Enable position independent code for shared library compatibility
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Try to find installed coreutils first, fallback to local source if not found
find_package(coreutils QUIET)
if(coreutils_FOUND)
    message(STATUS "Using system-installed coreutils library")
    set(COREUTILS_SOURCE "system")
else()
    # Check if local coreutils directory exists
    set(COREUTILS_LOCAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../coreutils")
    if(EXISTS "${COREUTILS_LOCAL_DIR}/CMakeLists.txt")
        if(NOT TARGET coreutils)
            # Dependencies automatically follow main project build type
            message(STATUS "Building coreutils in ${CMAKE_BUILD_TYPE} mode (auto-follow)")
            add_subdirectory(../coreutils coreutils_build)
        endif()
        message(STATUS "Using local coreutils from sibling directory")
        set(COREUTILS_SOURCE "local")
    else()
        # Neither system nor local coreutils found - show helpful error
        message(STATUS "")
        message(STATUS "========== DEPENDENCY ERROR ==========")
        message(FATAL_ERROR 
            "Cannot find coreutils dependency!\n"
            "\n"
            "Searched locations:\n"
            "  1. System installation (using find_package coreutils)\n"
            "  2. Local directory: ${COREUTILS_LOCAL_DIR}\n"
            "\n"
            "Solution - Download coreutils to sibling directory:\n"
            "\n"
            "    cd ${CMAKE_CURRENT_SOURCE_DIR}/..\n"
            "    git clone ssh://git@github.com/lmshao/coreutils.git\n"
            "\n"
            "    # OR if SSH is not available:\n"
            "    git clone https://github.com/lmshao/coreutils.git\n"
            "\n"
            "Repository Information:\n"
            "  📍 GitHub: https://github.com/lmshao/coreutils\n"
            "  🔑 SSH:    ssh://git@github.com/lmshao/coreutils.git\n"
            "\n"
            "After downloading, re-run: cmake .."
        )
    endif()
endif()

# Try to find installed network library, fallback to local source if not found
find_package(network QUIET)
if(network_FOUND)
    message(STATUS "Using system-installed network library")
    set(NETWORK_SOURCE "system")
else()
    # Check if local network directory exists
    set(NETWORK_LOCAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../network")
    if(EXISTS "${NETWORK_LOCAL_DIR}/CMakeLists.txt")
        if(NOT TARGET network)
            # Dependencies automatically follow main project build type
            message(STATUS "Building network in ${CMAKE_BUILD_TYPE} mode (auto-follow)")
            add_subdirectory(../network network_build)
        endif()
        message(STATUS "Using local network from sibling directory")
        set(NETWORK_SOURCE "local")
    else()
        # Neither system nor local network found - show helpful error
        message(STATUS "")
        message(STATUS "========== DEPENDENCY ERROR ==========")
        message(FATAL_ERROR 
            "Cannot find network dependency!\n"
            "\n"
            "Searched locations:\n"
            "  1. System installation (using find_package network)\n"
            "  2. Local directory: ${NETWORK_LOCAL_DIR}\n"
            "\n"
            "Solution - Download network to sibling directory:\n"
            "\n"
            "    cd ${CMAKE_CURRENT_SOURCE_DIR}/..\n"
            "    git clone ssh://git@github.com/lmshao/network.git\n"
            "\n"
            "    # OR if SSH is not available:\n"
            "    git clone https://github.com/lmshao/network.git\n"
            "\n"
            "Repository Information:\n"
            "  📍 GitHub: https://github.com/lmshao/network\n"
            "  🔑 SSH:    ssh://git@github.com/lmshao/network.git\n"
            "\n"
            "After downloading, re-run: cmake .."
        )
    endif()
endif()

# Source files and include directories
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/core)

# Add all subdirectories in src to include path
file(GLOB_RECURSE SRC_DIRS LIST_DIRECTORIES true "${PROJECT_SOURCE_DIR}/src/*")
foreach(DIR ${SRC_DIRS})
    if(IS_DIRECTORY ${DIR})
        include_directories(${DIR})
    endif()
endforeach()

# Monolithic mode: include all sources
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/core/main.cpp")

# Validate build options
if(NOT BUILD_STATIC_LIBS AND NOT BUILD_SHARED_LIBS)
    message(FATAL_ERROR "At least one of BUILD_STATIC_LIBS or BUILD_SHARED_LIBS must be ON")
endif()

# Compiler flags
if(MSVC)
    # MSVC specific flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Od /DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /DNDEBUG /DRELEASE")
else()
    # GCC/Clang flags for Linux/Unix
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error -Wno-format-truncation -Wno-unused-result")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -DRELEASE")
endif()

# Print build configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "C++ flags for ${CMAKE_BUILD_TYPE}: ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")

# Find threads (required for network operations)
find_package(Threads REQUIRED)

# Find X11 libraries for Linux screen capture
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    if(X11_FOUND)
        message(STATUS "Found X11 libraries: ${X11_LIBRARIES}")
    endif()
endif()

# Print source file summary
list(LENGTH SOURCES SOURCE_COUNT)
message(STATUS "Found ${SOURCE_COUNT} library source files")

# Static library
if(BUILD_STATIC_LIBS)
    add_library(remote-desk_static STATIC ${SOURCES})
    
    target_include_directories(remote-desk_static PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    # Link dependencies - prefer static to static linking
    if(coreutils_FOUND)
        target_link_libraries(remote-desk_static PUBLIC 
            $<IF:$<TARGET_EXISTS:coreutils::coreutils-static>,coreutils::coreutils-static,coreutils::coreutils>
        )
    else()
        target_link_libraries(remote-desk_static PUBLIC coreutils-static)
    endif()
    
    if(network_FOUND)
        target_link_libraries(remote-desk_static PUBLIC 
            $<IF:$<TARGET_EXISTS:network::network_static>,network::network_static,network::network>
        )
    else()
        target_link_libraries(remote-desk_static PUBLIC network_static)
    endif()
    
    target_link_libraries(remote-desk_static PUBLIC Threads::Threads)
    
    # Link DirectX libraries on Windows
    if(WIN32)
        target_link_libraries(remote-desk_static PUBLIC dxgi d3d11)
    endif()
    
    # Link X11 libraries on Linux
    if(UNIX AND NOT APPLE AND X11_FOUND)
        target_link_libraries(remote-desk_static PUBLIC ${X11_LIBRARIES})
    endif()
    
    set_target_properties(remote-desk_static PROPERTIES
        OUTPUT_NAME remote-desk
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    
    # Create alias for internal use
    add_library(remote-desk::remote-desk_static ALIAS remote-desk_static)
    message(STATUS "Building static library: libremote-desk.a")
endif()

# Shared library  
if(BUILD_SHARED_LIBS)
    add_library(remote-desk_shared SHARED ${SOURCES})
    
    target_include_directories(remote-desk_shared PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    # Link dependencies - prefer shared to shared linking, fallback to static
    if(coreutils_FOUND)
        target_link_libraries(remote-desk_shared PUBLIC 
            $<IF:$<TARGET_EXISTS:coreutils::coreutils-shared>,coreutils::coreutils-shared,coreutils::coreutils>
        )
    else()
        target_link_libraries(remote-desk_shared PUBLIC coreutils-shared)
    endif()
    
    if(network_FOUND)
        target_link_libraries(remote-desk_shared PUBLIC 
            $<IF:$<TARGET_EXISTS:network::network_shared>,network::network_shared,network::network>
        )
    else()
        target_link_libraries(remote-desk_shared PUBLIC network_shared)
    endif()
    
    target_link_libraries(remote-desk_shared PUBLIC Threads::Threads)
    
    # Link DirectX libraries on Windows
    if(WIN32)
        target_link_libraries(remote-desk_shared PUBLIC dxgi d3d11)
    endif()
    
    # Link X11 libraries on Linux
    if(UNIX AND NOT APPLE AND X11_FOUND)
        target_link_libraries(remote-desk_shared PUBLIC ${X11_LIBRARIES})
    endif()
    
    set_target_properties(remote-desk_shared PROPERTIES
        OUTPUT_NAME remote-desk
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    
    # Create alias for internal use
    add_library(remote-desk::remote-desk_shared ALIAS remote-desk_shared)
    message(STATUS "Building shared library: libremote-desk.so")
endif()

# Create default alias for examples and tests
if(BUILD_STATIC_LIBS)
    add_library(remote-desk ALIAS remote-desk_static)
    set(DEFAULT_LIBRARY_TYPE "static")
else()
    add_library(remote-desk ALIAS remote-desk_shared)
    set(DEFAULT_LIBRARY_TYPE "shared")
endif()

message(STATUS "Examples and tests will use ${DEFAULT_LIBRARY_TYPE} library")

# Build examples
if(BUILD_EXAMPLES)
    message(STATUS "Building Remote Desk examples...")
    
    # Create examples directory in build folder
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
    
    # Find all example source files
    file(GLOB EXAMPLE_SOURCES "examples/*.cpp")
    
    # Create executables for each example
    foreach(EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
        # Get filename without extension
        get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)
        
        # Create executable with original filename
        add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCE})
        
        # Set include directories for examples
        target_include_directories(${EXAMPLE_NAME} PRIVATE 
            ${PROJECT_SOURCE_DIR}/src
        )
        
        # Link to the default library (static or shared)
        target_link_libraries(${EXAMPLE_NAME} PRIVATE remote-desk)
        
        # Set output directory
        set_target_properties(${EXAMPLE_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
        )
        
        message(STATUS "Remote Desk Example target: ${EXAMPLE_NAME}")
    endforeach()
endif()

# Build service plugins as separate .so files
if(BUILD_PLUGINS)
    message(STATUS "Building service plugins...")
    
    # Create plugins directory
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)
    
    # Build each service as a separate plugin
    foreach(PLUGIN_SOURCE ${PLUGIN_SOURCES})
        get_filename_component(PLUGIN_NAME ${PLUGIN_SOURCE} NAME_WE)
        
        # Create plugin library
        add_library(${PLUGIN_NAME}_plugin SHARED ${PLUGIN_SOURCE})
        
        # Set plugin properties
        set_target_properties(${PLUGIN_NAME}_plugin PROPERTIES
            OUTPUT_NAME ${PLUGIN_NAME}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
            PREFIX ""  # Remove lib prefix
        )
        
        # Link to main library
        if(BUILD_SHARED_LIBS)
            target_link_libraries(${PLUGIN_NAME}_plugin PRIVATE remote-desk_shared)
        else()
            target_link_libraries(${PLUGIN_NAME}_plugin PRIVATE remote-desk_static)
        endif()
        
        message(STATUS "Plugin target: ${PLUGIN_NAME}_plugin -> plugins/${PLUGIN_NAME}.so")
    endforeach()
endif()

# Build applications - Remote Desk main executable
add_executable(remote-desk-svc src/core/main.cpp)
target_include_directories(remote-desk-svc PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

# Force link all library code to ensure service auto-registration
if(BUILD_STATIC_LIBS)
    if(MSVC)
        # Use MSVC-specific option for whole archive linking
        target_link_libraries(remote-desk-svc PRIVATE 
            $<LINK_LIBRARY:WHOLE_ARCHIVE,remote-desk_static>
        )
    else()
        # Use GNU linker options for GCC/Clang
        target_link_libraries(remote-desk-svc PRIVATE 
            -Wl,--whole-archive remote-desk_static -Wl,--no-whole-archive
        )
    endif()
else()
    target_link_libraries(remote-desk-svc PRIVATE remote-desk)
endif()
set_target_properties(remote-desk-svc PROPERTIES
    OUTPUT_NAME remote-desk
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
message(STATUS "Building Remote Desk main application")

# Build tests
if(BUILD_TESTS)
    enable_testing()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        message(STATUS "Building Remote Desk unit tests...")
        add_subdirectory(tests)
    else()
        message(STATUS "Tests directory not found, skipping tests")
    endif()
endif()

# Print build configuration summary
message(STATUS "")
message(STATUS "========== Remote Desk Library Build Configuration ==========")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Coreutils source: ${COREUTILS_SOURCE}")
message(STATUS "Network source: ${NETWORK_SOURCE}")
message(STATUS "Build static library: ${BUILD_STATIC_LIBS}")
message(STATUS "Build shared library: ${BUILD_SHARED_LIBS}")
message(STATUS "Build examples: ${BUILD_EXAMPLES}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "Build plugins: ${BUILD_PLUGINS}")
message(STATUS "===============================================================")
message(STATUS "")