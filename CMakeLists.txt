cmake_minimum_required(VERSION 3.16)
project(duckmuxpeg VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(LIBAV REQUIRED
    libavcodec
    libavformat
    libavutil
    libswscale
    libswresample
)

pkg_check_modules(OBS obs-studio)
if(OBS_FOUND)
    message(STATUS "OBS Studio development files detected - OBS plugin will be built")
else()
    message(WARNING "OBS Studio development files not found - OBS plugin will be skipped")
endif()

add_subdirectory(src/common)
add_subdirectory(src/statmux)
add_subdirectory(src/broadcast)
add_subdirectory(src/encoder)
add_subdirectory(src/obs-ui)

if(OBS_FOUND)
    add_subdirectory(src/obs-plugin)
endif()

add_executable(duckmuxpeg src/main.cpp)

target_include_directories(duckmuxpeg PRIVATE
    src/statmux
    src/obs-plugin
    src/common
)

target_link_libraries(duckmuxpeg
    statmux_engine
    common_utils
    modern_mpeg2_encoder
    broadcast_compliance
    obs_ui_components
    ${LIBAV_LIBRARIES}
    Threads::Threads
)

target_compile_options(duckmuxpeg PRIVATE ${LIBAV_CFLAGS_OTHER})
target_link_directories(duckmuxpeg PRIVATE ${LIBAV_LIBRARY_DIRS})

install(TARGETS duckmuxpeg DESTINATION bin)
install(FILES README.md DESTINATION share/doc/duckmuxpeg)
