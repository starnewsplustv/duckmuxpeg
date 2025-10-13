cmake_minimum_required(VERSION 3.16)
project(duckmuxpeg VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(_libav_vendored OFF)
set(_local_libav_prefix "${CMAKE_SOURCE_DIR}/third_party/libav/root/usr")
if(EXISTS "${_local_libav_prefix}")
    list(PREPEND CMAKE_PREFIX_PATH "${_local_libav_prefix}")
    set(_libav_vendored ON)
endif()

set(_libav_pkgconfig_dirs
    "${CMAKE_SOURCE_DIR}/third_party/libav/root/usr/lib/x86_64-linux-gnu/pkgconfig"
    "${CMAKE_SOURCE_DIR}/third_party/libav/root/usr/lib/pkgconfig"
    "${CMAKE_SOURCE_DIR}/third_party/libav/root/usr/share/pkgconfig"
)

foreach(_pkg_dir IN LISTS _libav_pkgconfig_dirs)
    if(EXISTS "${_pkg_dir}")
        if(DEFINED ENV{PKG_CONFIG_PATH} AND NOT "$ENV{PKG_CONFIG_PATH}" STREQUAL "")
            set(ENV{PKG_CONFIG_PATH} "${_pkg_dir}:$ENV{PKG_CONFIG_PATH}")
        else()
            set(ENV{PKG_CONFIG_PATH} "${_pkg_dir}")
        endif()
    endif()
endforeach()

find_package(PkgConfig QUIET)
find_package(Threads REQUIRED)

set(LIBAV_FOUND OFF)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBAV QUIET
        libavcodec
        libavformat
        libavutil
        libswscale
        libswresample
    )
    if(LIBAV_FOUND)
        if(_libav_vendored)
            message(STATUS "FFmpeg development libraries detected via third_party/libav")
        else()
            message(STATUS "FFmpeg development libraries detected - enabling libav integration")
        endif()
    else()
        message(WARNING "FFmpeg development libraries not found - continuing without libav support")
        set(LIBAV_FOUND OFF)
        set(LIBAV_LIBRARIES "")
        set(LIBAV_CFLAGS_OTHER "")
        set(LIBAV_LIBRARY_DIRS "")
    endif()

    pkg_check_modules(OBS QUIET obs-studio)
else()
    message(WARNING "pkg-config not available - skipping detection of optional dependencies")
    set(LIBAV_FOUND OFF)
    set(LIBAV_LIBRARIES "")
    set(LIBAV_CFLAGS_OTHER "")
    set(LIBAV_LIBRARY_DIRS "")
    set(OBS_FOUND OFF)
endif()

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
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/statmux
    ${CMAKE_SOURCE_DIR}/src/obs-plugin
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_libraries(duckmuxpeg
    statmux_engine
    common_utils
    modern_mpeg2_encoder
    broadcast_compliance
    obs_ui_components
    Threads::Threads
)

if(LIBAV_FOUND)
    target_link_libraries(duckmuxpeg PRIVATE ${LIBAV_LIBRARIES})
    target_compile_options(duckmuxpeg PRIVATE ${LIBAV_CFLAGS_OTHER})
    target_link_directories(duckmuxpeg PRIVATE ${LIBAV_LIBRARY_DIRS})
endif()

install(TARGETS duckmuxpeg DESTINATION bin)
install(FILES README.md DESTINATION share/doc/duckmuxpeg)
