include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
        src/get.cpp
        src/list.cpp
        src/server.cpp include/put.h)

add_executable(server ${SOURCES})
target_link_libraries(server LINK_PUBLIC common)
#set (PROJECT_LINK_LIBS libcommon.so)