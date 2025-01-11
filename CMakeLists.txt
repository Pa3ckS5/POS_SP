cmake_minimum_required(VERSION 3.10)
project(RandomWalk)

set(CMAKE_C_STANDARD 99)

# server
add_executable(server server.c simulation.c socket.c)

# klient
add_executable(client client.c socket.c client_io.c)

# build
add_custom_target(build_all DEPENDS server client)

# mazanie
add_custom_target(clean_all COMMAND ${CMAKE_COMMAND} -E remove server client)