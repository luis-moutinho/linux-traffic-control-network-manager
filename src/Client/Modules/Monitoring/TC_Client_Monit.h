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

/**	@file TC_Client_Monit.h
*	@brief Function prototypes for the client monitoring module
*
*	This file contains the prototypes of the functions for the client monitoring module. 
*	This module periodically sends heartbeat messages to the server monitoring module to keep the node registered in the network.
*	It also (using the discovery module) periodically checks for the server. If the server is disconnected from the network this module raises
*	a signal to close the client.Internal module
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef TCCLIENTMONIT_H
#define TCCLIENTMONIT_H

#include "TC_Data_Types.h"

/**	
*	@brief Starts the client monitoring module
*
*	Creates the necessary sockets to comunicate with the server monitoring module.
*	Creates one thread to periodically send heartbeat messages and another to periodically check for the server
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
int tc_client_monit_init( char *ifface, unsigned int node_id, NET_ADDR *server );

/**
*	@brief Closes the client monitoring module
*
*	Closes all sockets and destroys the threads
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_monit_close( void );

#endif
