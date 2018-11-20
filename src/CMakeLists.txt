add_subdirectory (usb)
add_subdirectory (libui)


add_executable (nlsusb main.cpp mainview.cpp)
target_link_libraries(nlsusb usbcontext)
target_link_libraries(nlsusb libui)
target_link_libraries(nlsusb ${LIBUSB_LIBRARIES})
target_link_libraries(nlsusb ${UDEV_LIBRARIES})
target_link_libraries(nlsusb ${NCURSES_LIBRARIES})