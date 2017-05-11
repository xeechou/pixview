cmake_minimum_required(VERSION 3.1.0)

project(tcairo
  VERSION 0.1
  LANGUAGES C CXX)

set(CMAKE_C_FLAGS "-Wall -g")

add_definitions(
  -D_GNU_SOURCE
  )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -g")

#this is the one you missed
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${PROJECT_SOURCE_DIR}/CMake")

find_package(PkgConfig REQUIRED)
find_package(Cairo REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem)
pkg_check_modules(Magick++ GraphicsMagick++>=1.0)

include_directories(${CAIRO_INCLUDE_DIR})
include_directories(${SDL2_INCLUDE_DIR})
include_directories(${SDL2_IMAGE_INCLUDE_DIR})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")


include_directories(${Magick++_INCLUDE_DIRS})
add_subdirectory(src)

#add_executable(cairo_draw cairo_load_image_and_show.cpp)
