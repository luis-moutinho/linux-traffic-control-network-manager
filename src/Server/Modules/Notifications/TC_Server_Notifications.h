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

/**	@file TC_Server_Notifications.h
*	@brief Function prototypes for the server notifications module
*
*	This file contains the prototypes of the functions for the server notifications module. 
*	This module contains the means to send notification messages to the client notifications module when an event occurs (I.E node registered in the network).
*	Internal module
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef TCSERVERNOTIFICATIONS_H
#define TCSERVERNOTIFICATIONS_H

/**	
*	@brief Starts the server notifications module
*
*	Initializes the module and creates the necessary sockets to comunicate with the clients notifications modules.
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
int tc_server_notifications_init( NET_ADDR *server_remote );

/**
*	@brief Sends an event trigger message
*
*	Sends an event message to the clients notifications modules.
*	In this current version only nodes registration and deregistration events are triggered
*
*	@param[in] event 	The occured event code. Must be one of the defined codes (EVENT_NODE_PLUG/EVENT_NODE_UNPLUG)
*	@param[in] node		The address of the node entry which triggered the event. Must not be a NULL pointer
*
*	@pre			assert( node );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*
*	@note			In the current version only node registration/unregistration events trigger a notification
*
*	@todo			Implement more notification events (I.E topic created/destroyed/changed)
*/
int tc_server_notifications_send_node_event( unsigned char event, NODE_ENTRY *node );

/**	
*	@brief Closes the server notifications module
*
*	Closes the module and sockets
*
*	@pre				None
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*/
int tc_server_notifications_close( void );

#endif
