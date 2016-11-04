include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
        src/client.cpp)

add_executable(client ${SOURCES})
target_link_libraries(client LINK_PUBLIC common)
#set (PROJECT_LINK_LIBS libcommon.so)
