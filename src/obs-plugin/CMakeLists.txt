add_library(obs-duckmuxpeg-plugin SHARED
    OBSOutputPlugin.cpp
    OBSStatMuxOutput.cpp
    OBSWebSocketClient.cpp
    OBSSceneMonitor.cpp
)

target_include_directories(obs-duckmuxpeg-plugin PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/statmux
)

target_link_libraries(obs-duckmuxpeg-plugin
    statmux_engine
    common_utils
    ${OBS_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

# Set plugin properties
set_target_properties(obs-duckmuxpeg-plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "obs-duckmuxpeg"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/obs-plugins"
)

# Install plugin
install(TARGETS obs-duckmuxpeg-plugin
    LIBRARY DESTINATION obs-plugins
)