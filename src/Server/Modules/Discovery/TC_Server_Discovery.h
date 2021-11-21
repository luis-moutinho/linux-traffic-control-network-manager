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

/**	@file TC_Server_Discovery.h
*	@brief Function prototypes for the server discovery module
*
*	This file contains the prototype of the functions for the server discovery module. 
*	This module sends periodically discovery messages to all the possible listening nodes. This messages
*	contain the servers configured address. 
*	Internal module
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef TCSERVERDISCOVERY_H
#define TCSERVERDISCOVERY_H

#include "TC_Data_Types.h"

/**	
*	@brief Starts the server discovery module
*
*	Initializes the module and creates the necessary sockets to comunicate with the clients discovery modules.
*	Creates a thread that periodically sends messages with the servers configured address to the remote nodes
*	and another thread to send to the local nodes.
*
*	@param[in] server_remote	The server configured remote address. Must not be a NULL pointer	
*
*	@pre				assert( server_remote );
*	@pre				assert( server_remote->name_ip );
*	@pre				assert( server_remote->port );
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*/
int tc_server_discovery_init( NET_ADDR *server_remote );

/**	
*	@brief Closes the server discovery module
*
*	Stops both threads and closes the module and sockets
*
*	@pre				None
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*/
int tc_server_discovery_close( void );

#endif
