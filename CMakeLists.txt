cmake_minimum_required(VERSION 3.16)
project(duckmuxpeg VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Find FFmpeg components
pkg_check_modules(LIBAV REQUIRED 
    libavcodec 
    libavformat 
    libavutil 
    libswscale 
    libswresample
)

# Find OBS Studio
pkg_check_modules(OBS REQUIRED obs-studio)

# Include directories
include_directories(
    src/statmux
    src/obs-plugin
    src/common
    ${LIBAV_INCLUDE_DIRS}
    ${OBS_INCLUDE_DIRS}
)

# Add subdirectories
add_subdirectory(src/statmux)
add_subdirectory(src/obs-plugin)
add_subdirectory(src/common)

# Create main executable
add_executable(duckmuxpeg src/main.cpp)

target_link_libraries(duckmuxpeg
    statmux_engine
    common_utils
    ${LIBAV_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

# Compiler flags
target_compile_options(duckmuxpeg PRIVATE ${LIBAV_CFLAGS_OTHER})
target_link_directories(duckmuxpeg PRIVATE ${LIBAV_LIBRARY_DIRS})

# Install targets
install(TARGETS duckmuxpeg DESTINATION bin)
install(FILES README.md DESTINATION share/doc/duckmuxpeg)