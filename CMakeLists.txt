cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

message(STATUS "MODULES:${CMAKE_MODULE_PATH}")

#set(CMAKE_CXX_FLAGS "-pthread -std=c++11 -O2 -Wall -Wextra")
#set(CMAKE_CXX_FLAGS "-pthread -std=c++11 -O3 -Wall -Wextra")
set(CMAKE_CXX_FLAGS "-pthread -std=c++11 -g -Wall -Wextra")

find_package(Poppler REQUIRED)
find_package(GLIB2 REQUIRED)
find_package(Cairo REQUIRED)

include_directories(${GLIB2_INCLUDE_DIR})
include_directories(${CAIRO_INCLUDE_DIRS})
include_directories(${POPPLER_INCLUDE_DIRS})

MESSAGE( STATUS "Inlcudes: " ${GLIB2_INCLUDE_DIR} )

add_subdirectory(src)