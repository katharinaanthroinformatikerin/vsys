set(SERVER_SOURCES server.cpp)

set(CLIENT_SOURCES client.cpp)



add_executable(server ${SERVER_SOURCES})

target_link_libraries(server ldap lber)

add_executable(client ${CLIENT_SOURCES})