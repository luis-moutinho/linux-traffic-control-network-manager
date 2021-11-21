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

/**	@file TC_Server.h
*	@brief Function prototypes for the server side API
*
*	This file contains the prototypes of the functions for the server API
*	to be used by the application. This module creates the necessary sockets and threads to receive and handle requests from clients.
*	This module also initializes all the necessary internal control modules necessary to resolve the requests. Top module
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef TCSERVER_H
#define TCSERVER_H

/**	
*	@brief Starts the server module
*
*	Initializes all the server required modules and creates all the necessary sockets to receive requests from clients.
*	Creates a thread to receive and handle the received requests
*
*	@param[in] ifface	The NIC interface to be used to connect to the network. Must not be a NULL pointer
*	@param[in] server_port 	The port address number to be used for the server. Must be greater than 0
*
*	@pre			None
*
*	@return 		Upon successful return : ERR_OK (0)
*	@return 		Upon output error : An error code (<0)
*/
int tc_server_init( char *ifface, unsigned int server_port );

/**	
*	@brief Closes the server module
*
*	Stops the request handling thread, closes all sockets and all the server modules
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_server_close( void );

#endif
