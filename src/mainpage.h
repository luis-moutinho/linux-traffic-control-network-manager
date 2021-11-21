/*This file is part of LTCNM (Linux Traffic Control Network Manager).

    LTCNM is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LTCNM is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with LTCNM.  If not, see <http://www.gnu.org/licenses/>.
*/

/** 
*	@mainpage Main Page
*
* 	@authors 	Luís Emanuel Moutinho da Silva\n
*			luis.silva.ua@gmail.com
*
* 	@section intro Introduction
*
* 	This package provides the source code of a network manager based on the in-built linux traffic control
* 	mechanisms.
*
* 	This package is structured into 5 main directories and 1 makefile :
*		- doc		: 	This folder contains this documentation
*
*		- Makefile 	: 	This makefile will build the project libraries which can then be used to build applications.
*					To build the libraries just run "make" in a terminal. Running "make clean" will clean all the built/copied files
*
*		- libs 		: 	The library files used to build applications will be located here after they were built
*
*		- Includes 	: 	All header files for external access will be located here after the libraries were built
*
*		- src 		: 	All the source files and headers not meant for external access reside in this folder
*
*		- tests 	: 	A simple usage example is located here. To build it just use the makefile that is also inside this folder and run "make" in a terminal. 
*					A run example is provided in the help section (run "./TC_API.test -h")
*
*	<hr>	
*		@section features Current Features
*		This current LTCNM version supports :
*			- Abstract network topic with 3 properties that identifies and characterizes them ( Topic ID, Topic Message Size, Topic Message Period )
*			- Registration of nodes in the network with a unique ID
*			- Comunication between nodes using the producer-consumer paradigm ( UDP protocol used )
*			- Registration of multiple nodes as consumers and/or producers of a given Topic ( multi-producer / multi-consumer paradigm )
*			- Two different node types :
*				- A server node ( the network manager who manages the network, accepts requests from nodes, keeps the topic and node database, etc.. )
*				- A client node ( the regular nodes who makes requests to the server and comunicates with other clients through topics )
*			- One client can be instantiated in the same physical node as the server ( the network manager )
*			- Each message sent to a topic is put in a pre-allocated linux traffic control queue (residing locally in each producer node) with limited bandwidth ( allocated according to the topic properties )
*			thus making it possible to control the bandwidth in all the nodes
*			- Topic messages bandwidth is independent for each topic by assigning one queue to each topic
*			- Multicast is used so we can add many consumers to the same topic without overloading the producers uplink bandwidth
*			- Detection of the removal from the network of a client or server
*			- Event (such as node registration/unregistration) notifications are sent to all the nodes 
*			- You tell me!!  
*
* 	<hr>
* 		@section notes Release notes
* 			- V1.0.0
*				- Initial release 
*				- Source code and documentation released
*/
