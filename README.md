This package provides the source code of a network manager based on the in-built linux traffic control mechanisms.

This package is structured into 5 main directories and 1 makefile :

- doc : This folder contains this documentation
- Makefile : This makefile will build the project libraries which can then be used to build applications. To build the libraries just run "make" in a terminal. Running "make clean" will clean all the built/copied files
- libs : The library files used to build applications will be located here after they were built
- Includes : All header files for external access will be located here after the libraries were built
- src : All the source files and headers not meant for external access reside in this folder
- tests : A simple usage example is located here. To build it just use the makefile that is also inside this folder and run "make" in a terminal. A run example is provided in the help section (run "./TC_API.test -h")

:warning:
The last development work on this project was done circa 2013, for Linux OS Ubuntu 12.04 LTS (kernel version 3.2.0). Thus, the project may not properly compile in newer kernel versions.