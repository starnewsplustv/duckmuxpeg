add_library(statmux_engine STATIC
    StatMuxEngine.cpp
    InputHandler.cpp
    OutputHandler.cpp
    BitrateController.cpp
    StreamProcessor.cpp
    QualityAnalyzer.cpp
    TransportStream.cpp
    PlaylistManager.cpp
    TrafficAccountant.cpp
    LiveSourceScheduler.cpp
    PlatformManager.cpp
)

target_include_directories(statmux_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(statmux_engine
    common_utils
    ${CMAKE_THREAD_LIBS_INIT}
)

if(LIBAV_FOUND)
    target_link_libraries(statmux_engine PRIVATE ${LIBAV_LIBRARIES})
    target_compile_options(statmux_engine PRIVATE ${LIBAV_CFLAGS_OTHER})
    target_link_directories(statmux_engine PRIVATE ${LIBAV_LIBRARY_DIRS})
endif()
