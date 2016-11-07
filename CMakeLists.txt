cmake_minimum_required(VERSION 2.8)

project(vsys)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Notwendig für LDAP

set(GCC_COVERAGE_COMPILE_FLAGS "-DLDAP_DEPRECATED")

set(GCC_COVERAGE_LINK_FLAGS    "-lldap -llber")

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )

set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}" )

# Ende LDAP

set(SERVER_SOURCES src/server.cpp)

set(CLIENT_SOURCES src/client.cpp)

add_executable(server ${SERVER_SOURCES})

add_executable(client ${CLIENT_SOURCES})
