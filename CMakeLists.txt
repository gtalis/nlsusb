#
# nlsusb
#
project(nlsusb C CXX)

cmake_minimum_required(VERSION 2.8)


set (nlsusb_VERSION_MAJOR	0)
set (nlsusb_VERSION_MINOR	0)
set (nlsusb_VERSION_PATCH	1)


# Set C++11 as a minimum required standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


###############################################################################
#
# dependencies
#
###############################################################################
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

add_subdirectory (${PROJECT_SOURCE_DIR}/src/usb)
add_subdirectory (${PROJECT_SOURCE_DIR}/src/libui)


add_executable (nlsusb src/main.cpp src/mainview.cpp)
target_link_libraries(nlsusb usbcontext)
target_link_libraries(nlsusb libui)
target_link_libraries(nlsusb ${LIBUSB_LIBRARIES})
target_link_libraries(nlsusb ${UDEV_LIBRARIES})
target_link_libraries(nlsusb ${NCURSES_LIBRARIES})

