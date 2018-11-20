project(nlsusb)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBUSB libusb-1.0 REQUIRED)
pkg_check_modules(UDEV libudev REQUIRED)
pkg_check_modules(NCURSES ncurses REQUIRED)

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/src/usb")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${LIBUSB_INCLUDE_DIRS})
include_directories(${UDEV_INCLUDE_DIRS})
include_directories(${NCURSES_INCLUDE_DIRS})

add_subdirectory(${PROJECT_SOURCE_DIR}/src)

