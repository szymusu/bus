cmake_minimum_required(VERSION 3.30)
project(bus C)

set(CMAKE_C_STANDARD 11)

add_executable(protocol protocol.c)
add_executable(server server.c)
add_executable(client client.c)
