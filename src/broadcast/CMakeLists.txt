add_library(broadcast_compliance STATIC
    ATSCCompliance.cpp
    PSIPTables.cpp
    TransportStreamValidator.cpp
    ClosedCaptioning.cpp
)

target_include_directories(broadcast_compliance PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_libraries(broadcast_compliance
    common_utils
    ${CMAKE_THREAD_LIBS_INIT}
)