add_library(broadcast_compliance STATIC
    ATSCCompliance.cpp
)

target_include_directories(broadcast_compliance PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_libraries(broadcast_compliance
    common_utils
    Threads::Threads
)
