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

/**	@file TC_Client_Discovery.h
*	@brief Function prototypes for the client discovery module
*
*	This file contains the prototypes of the functions for the client discovery
*	module. This module implements mechanisms to discover a server and retrieve its address. This server can be local to the node or in the network.
*	Internal module
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef TCCLIENTDISCOVERY_H
#define TCCLIENTDISCOVERY_H

#include "TC_Data_Types.h"

/**
*	@brief Starts the client discovery module
*
*	Initializes the module and creates all the required sockets for the server discovery
*
*	@pre			assert( ifface );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_discovery_init( char *ifface );

/**
*	@brief Searches for a server
*
*	Searches for a local or remote server. This is done by listenning to pre-defined addresses for 
*	a discovery type message containing the server address
*
*	@param[in] timeout	Maximum time interval (in ms) to wait for the server discovery. If 0 blocks indefinitely. Must be equal or greater than 0
*	@param[out] ret_server	The buffer where to store the discovered server address. Must not be a NULL pointer
*	@pre			assert( ret_server );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_discovery_find_server( unsigned int timeout, NET_ADDR *ret_server );

/**
*	@brief Closes the client discovery module
*
*	Closes all the created sockets and module
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_discovery_close( void );

#endif
