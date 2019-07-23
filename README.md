# nlsusb
=================================

Copyright (c) 2018-2019, [Gilles Talis](gilles.talis@gmail.com)

** nlsusb ** is a very basic ncurses-based **[lsusb](https://github.com/gregkh/usbutils)** viewer.
It provides a "nicer" (please quote nicer as in "the laser") output to the lsusb tool.

It is a non-optimised, quick and dirty tool that needs cleanup, code refactoring
and addition of fancy features like colors.

## Build from source

### Dependencies
* libusb
* udev
* ncurses

### Building
	mkdir build
	cd build
	cmake ..
	make
	

## Usage
	./nlsusb
	or if you have sudo rights
	sudo ./nlsusb
	
you will have access to more information about the plugged USB devices if you use the sudo version
