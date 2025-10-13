add_library(modern_mpeg2_encoder STATIC
    ModernMPEG2Encoder.cpp
)

target_include_directories(modern_mpeg2_encoder PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(modern_mpeg2_encoder
    common_utils
    Threads::Threads
)

if(LIBAV_FOUND)
    target_link_libraries(modern_mpeg2_encoder PRIVATE ${LIBAV_LIBRARIES})
    target_compile_options(modern_mpeg2_encoder PRIVATE ${LIBAV_CFLAGS_OTHER})
    target_link_directories(modern_mpeg2_encoder PRIVATE ${LIBAV_LIBRARY_DIRS})
endif()
