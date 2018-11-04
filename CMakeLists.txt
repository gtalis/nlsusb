project(nlsusb)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBUSB libusb REQUIRED)
pkg_check_modules(UDEV udev REQUIRED)

file(GLOB sources
	main.cpp
	mainview.cpp
	usbcontext.cpp
	usbdevice.cpp
)


include_directories(${LIBUSB_INCLUDE_DIRS})
include_directories(${UDEV_INCLUDE_DIRS})


add_executable(nlsusb ${sources})
target_link_libraries(nlsusb ${LIBUSB_LIBRARIES})
target_link_libraries(nlsusb ${UDEV_LIBRARIES})
