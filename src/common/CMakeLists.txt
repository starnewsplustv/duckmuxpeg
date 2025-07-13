add_library(common_utils STATIC
    Logger.cpp
    Config.cpp
    CircularBuffer.cpp
    Utils.cpp
)

target_include_directories(common_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(common_utils
    ${CMAKE_THREAD_LIBS_INIT}
)