# generated by cmake-generator
cmake_minimum_required(VERSION 3.16.3)

add_executable(clash-example main.c)

include(Tornado.cmake)
set_tornado(clash-example)

target_include_directories(clash-example PUBLIC ../include)

target_link_libraries(clash-example PUBLIC clash)
