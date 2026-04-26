# Building the Control Room Software

The control room is made with QT 6.7.2, make sure that you have QT 6.7.2 installed.
You will also need cmake and OpenGL installed.

The control room is designed to be built on Linux, it should also work on Mac, but that is untested.
Windows support is planned, but not yet added. I would recommend getting access to a Linux computer,
because WSL does not support interfacing with Serial.

## Installing Dependencies

You can install QT 6.7.2 from their website: https://www.qt.io/download-qt-installer-oss

You can install cmake and OpenGL on Debian based Distros using:

	sudo apt install build-essential cmake mesa-utils mesa-common-dev libglfw3 libglfw3-dev libglew-dev libglm-dev libx11-dev libxrandr-dev libxi-dev libxinerama-dev libxcursor-dev

## Configuring CMake

I am assuming that you have the root of the program open in the terminal.

First change the working directory to the `RocketEngineControlGUI` directory

	cd src/RocketEngineControlGUI

Next lets create a folder to store our compiled program, (aka build artifacts)

	mkdir build

Lastly, lets enter that directory and configure cmake

	cd build
	cmake ..
	cd ..

## Building the project

Assuming that there were no errors configuring the project, you can now run the build command

	cmake --build build

## Troubleshooting

### CMake: no program exists

Make sure that you ran the command above to install cmake 

### CMake: Qt6 cannot be found

Make sure you have Qt V6.7.2 installed, and have OpenGL installed via the command above.

### Other issues:

If you have trouble with anything, join the [Discord](https://discord.gg/eGm8ZMqRBy) and ask for help in the `Systems and Electronics` channel under the Rocket Engine Group.
