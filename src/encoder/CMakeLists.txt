add_library(modern_mpeg2_encoder STATIC
    ModernMPEG2Encoder.cpp
    LookaheadAnalyzer.cpp
    RateController.cpp
    MotionEstimation.cpp
    AdaptiveQuantizer.cpp
    SceneChangeDetector.cpp
    ParallelEncoder.cpp
    RAMBufferManager.cpp
    x264Integration.cpp
)

target_include_directories(modern_mpeg2_encoder PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_libraries(modern_mpeg2_encoder
    common_utils
    ${LIBAV_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

# Find x264 library for reference implementation
find_path(X264_INCLUDE_DIR x264.h)
find_library(X264_LIBRARY x264)

if(X264_INCLUDE_DIR AND X264_LIBRARY)
    target_include_directories(modern_mpeg2_encoder PRIVATE ${X264_INCLUDE_DIR})
    target_link_libraries(modern_mpeg2_encoder ${X264_LIBRARY})
    target_compile_definitions(modern_mpeg2_encoder PRIVATE HAVE_X264)
endif()

target_compile_options(modern_mpeg2_encoder PRIVATE ${LIBAV_CFLAGS_OTHER})
target_link_directories(modern_mpeg2_encoder PRIVATE ${LIBAV_LIBRARY_DIRS})