include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(usbcontext
	names.c
	names.h
	usbmisc.c
	usbmisc.h
	usbcontext.cpp
	usbdevice.cpp)
