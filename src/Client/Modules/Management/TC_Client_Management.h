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

/**	@file TC_Client_Management.h
*	@brief Function prototypes for the client management module
*
*	This file contains the prototypes of the functions for the client management
*	module. This module receives several types of requests from the server and handles them (I.E reservations requests,unbind requests, topic updates.. ).
*	Internal module
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef TCCLIENTMANAGEMENT_H
#define TCCLIENTMANAGEMENT_H

/**	
*	@brief Starts the client management module
*
*	Creates the necessary sockets to comunicate with the server management module.
*	Creates a thread that receives and handles requests from the server
*
*	@param[in] ifface	The NIC to use for networking. Must not be a NULL pointer
*	@param[in] node_id	The assigned node ID. Must be greater than 0
*	@param[in] server	The server address. Must not be a NULL pointer
*
*	@pre			assert( ifface );
*	@pre			assert( node_id );
*	@pre			assert( server );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_management_init( char *ifface, unsigned int node_id, NET_ADDR *server );

/**
*	@brief Closes the client management module
*
*	Closes all sockets and destroys the request handling thread
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_management_close( void );

#endif
