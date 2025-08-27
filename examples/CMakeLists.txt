# Build .cpp examples
file(GLOB CPP_EXAMPLES "*.cpp")
foreach(src IN LISTS CPP_EXAMPLES)
    string(REGEX REPLACE "^.*/([^/.]+).cpp" "\\1" target ${src})
    message(STATUS "CPP Example target: " ${target})
    add_executable(${target} ${src})
    target_link_libraries(${target} remote-desk)
endforeach(src)