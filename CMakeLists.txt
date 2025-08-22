cmake_minimum_required(VERSION 3.10)
project(remote-desk VERSION 0.0.1 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for tools like clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add subdirectories for libraries
add_subdirectory(src/discovery)

# Source files and include directories
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/log)
include_directories(${PROJECT_SOURCE_DIR}/src/core)

# Collect source files excluding discovery (now built as library)
file(GLOB_RECURSE SOURCE_FILES 
    "src/core/*.cpp"
    "src/log/*.cpp"
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Link with discovery library
target_link_libraries(${PROJECT_NAME} discovery)

# Set target properties
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

# Compiler options
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Set default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Build type specific options
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=1)
    target_compile_options(${PROJECT_NAME} PRIVATE -g -O0)
    message(STATUS "Debug build enabled")
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG=1)
    target_compile_options(${PROJECT_NAME} PRIVATE -O3)
    message(STATUS "Release build enabled")
endif()

# Install rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT runtime
)

# Print build information
message(STATUS "=== Build Configuration ===")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Source directory: ${PROJECT_SOURCE_DIR}")
message(STATUS "Binary directory: ${PROJECT_BINARY_DIR}")
message(STATUS "===========================")

