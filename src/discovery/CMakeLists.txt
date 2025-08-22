# Discovery module - Shared Library
add_library(discovery SHARED
    discovery.cpp
    discovery.h
)

# Set library properties
set_target_properties(discovery PROPERTIES
    OUTPUT_NAME "discovery"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Include directories
target_include_directories(discovery PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../log
)

# Compiler options
target_compile_options(discovery PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Build type specific options
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(discovery PRIVATE DEBUG=1)
    target_compile_options(discovery PRIVATE -g -O0)
else()
    target_compile_definitions(discovery PRIVATE NDEBUG=1)
    target_compile_options(discovery PRIVATE -O3)
endif()

# Install rules
install(TARGETS discovery
    LIBRARY DESTINATION lib
    COMPONENT runtime
)
