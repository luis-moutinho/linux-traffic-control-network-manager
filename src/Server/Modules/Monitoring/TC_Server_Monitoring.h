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

/**	@file TC_Server_Monitoring.h
*	@brief Function prototypes for the server monitoring module
*
*	This file contains the prototype of the functions for the server monitoring module. 
*	This module receives the heartbeat request messages from clients to reset their heartbeat counter. This module also periodically decrements
*	all nodes counter and signals a dead node when its counter reaches 0.
*	Internal module
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef TCSERVERMONIT_H
#define TCSERVERMONIT_H

/**	
*	@brief Starts the server monitoring module
*
*	Initializes the module and creates the necessary sockets to comunicate with the clients monitoring modules.
*	Creates a thread to receive the heartbeat reset requests from the clients and another thread to periodically
*	decrement all nodes heartbeat counter.
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
int tc_server_monitoring_init( NET_ADDR *server_remote );

/**	
*	@brief Closes the server monitoring module
*
*	Stops both threads and closes all the sockets and module.
*
*	@pre				None
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*/
int tc_server_monitoring_close( void );

#endif
