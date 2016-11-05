include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
        src/get.cpp
        src/list.cpp
        src/server.cpp include/put.h)

set(GCC_COVERAGE_COMPILE_FLAGS "-DLDAP_DEPRECATED")
set(GCC_COVERAGE_LINK_FLAGS    "-lldap -llber")

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}" )

add_executable(server ${SOURCES})
target_link_libraries(server LINK_PUBLIC common ldap lber)
#set (PROJECT_LINK_LIBS libcommon.so)


#add_library(common STATIC ${SOURCES})
#target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
