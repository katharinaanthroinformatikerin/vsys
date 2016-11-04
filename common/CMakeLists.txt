include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
        src/common.cpp
        src/config.cpp)

add_library(common SHARED ${SOURCES})
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})