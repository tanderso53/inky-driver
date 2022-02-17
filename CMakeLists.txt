cmake_minimum_required(VERSION 3.21)

add_library(pimoroni-inky-driver INTERFACE)

target_sources(pimoroni-inky-driver INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/inky.c)

target_include_directories(pimoroni-inky-driver INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/include)

set_target_properties(pimoroni-inky-driver PROPERTIES
  PUBLIC_HEADER ${CMAKE_CURRENT_LIST_DIR}/include/inky.h)

set_target_properties(pimoroni-inky-driver PROPERTIES
  PRIVATE_HEADER ${CMAKE_CURRENT_LIST_DIR}/include/inky-api.h)
