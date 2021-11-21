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

/**	@file TC_Client_Notifications.h
*	@brief Function prototypes for the client notifications module
*
*	This file contains the prototypes of the functions for the client notifications module. 
*	This module polls for notification messages sent by the server notifications module when an event occurs (I.E node registered in the network).
*	Internal module
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef TCCLIENTNOTIFICATIONS_H
#define TCCLIENTNOTIFICATIONS_H

#include "TC_Data_Types.h"

/**	
*	@brief Starts the client notifications module
*
*	Creates the necessary sockets to comunicate with the server notifications module.
*
*	@param[in] ifface	The NIC to use for networking. Must not be a NULL pointer
*	@param[in] server	The server address. Must not be a NULL pointer
*
*	@pre			assert( ifface );
*	@pre			assert( server );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_notifications_init( char *ifface, NET_ADDR *server );

/**	
*	@brief Polls for a notification message
*
*	Polls for a notification message sent by the server notifications module.
*	This call is used by the client tc_client_get_node_event( unsigned int timeout, unsigned char *ret_event, unsigned int *ret_node_id ) call
*
*	@param[in] timeout	Maximum time interval (in ms) to wait for a message. If 0 blocks indefinitely. Must be equal or greater than 0
*	@param[out] ret_msg	The buffer to store the polled message. Must not be a NULL pointer
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*
*	@note			In the current version only node registration/unregistration events trigger a notification
*
*	@todo			Implement more notification events (I.E topic created/destroyed/changed)
*/
int tc_client_notifications_get( unsigned int timeout, NET_MSG *ret_msg );

/**
*	@brief Closes the client notifications module
*
*	Closes all sockets.
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_notifications_close( void );

#endif
