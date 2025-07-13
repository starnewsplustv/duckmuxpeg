add_library(obs_ui_components STATIC
    BroadcastComplianceUI.cpp
    PSIPTableEditor.cpp
    ComplianceMonitorWidget.cpp
    ChannelManagerWidget.cpp
    EncoderControlUI.cpp
)

target_include_directories(obs_ui_components PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/broadcast
    ${CMAKE_SOURCE_DIR}/src/encoder
)

target_link_libraries(obs_ui_components
    common_utils
    broadcast_compliance
    modern_mpeg2_encoder
    ${OBS_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)