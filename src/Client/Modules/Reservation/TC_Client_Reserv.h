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

/**	@file TC_Client_Reserv.h
*	@brief Source code of the functions for the client reservation module
*
*	This file contains the prototypes of the functions for the client reservation module. 
*	This module is used by the management module to configure network reservations using the linux traffic control mechanism.
*	Internal module
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef TCCLIENTRESERV_H
#define TCCLIENTRESERV_H

#include "TC_Data_Types.h"

/**	
*	@brief Starts the client reservation module
*
*	Initializes the module and creates the reservations necessary to the comunication with the server
*
*	@param[in] ifface	The NIC to use for networking. Must not be a NULL pointer
*	@param[in] node_id	The assigned node ID. Must be greater than 0
*	@param[in] server_addr	The server address. Must not be a NULL pointer
*
*	@pre			assert( ifface );
*	@pre			assert( node_id );
*	@pre			assert( server_addr );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_reserv_init( char *ifface, unsigned int node_id, NET_ADDR *server_addr );

/**
*	@brief Closes the client reservation module
*
*	Frees all the reservations made and closes the module
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_reserv_close( void );

/**	
*	@brief Creates a network reservation
*
*	Creates a network reservation for the topic messages by creating a linux traffic control qdisc (with limited bandwidth) and filter.
*	This filter will assign all the messages with the topic destination address to the configured qdisc
*
*	@param[in] topic_id	The ID of the topic. Must be greater than 0
*	@param[in] topic_addr	The topic network address. Must not be a NULL pointer
*	@param[in] req_load	The reservation bandwidth ammount. Must be greater than 0
*
*	@pre			assert( topic_id );
*	@pre			assert( topic_addr );
*	@pre			assert( req_load );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*
*	@todo			Create missing error codes
*/
int tc_client_reserv_add( unsigned int topic_id, NET_ADDR *topic_addr, unsigned int req_load );

/**	
*	@brief Updates a network reservation
*
*	Updates an existing network reservation bandwidth
*
*	@param[in] topic_id	The ID of the topic. Must be greater than 0
*	@param[in] topic_addr	The topic network address. Must not be a NULL pointer
*	@param[in] req_load	The new reservation bandwidth ammount. Must be greater than 0
*
*	@pre			assert( topic_id );
*	@pre			assert( topic_addr );
*	@pre			assert( req_load );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*
*	@todo			Create missing error codes
*/
int tc_client_reserv_set( unsigned int topic_id, NET_ADDR *topic_addr, unsigned int req_load );

/**	
*	@brief Frees a network reservation
*
*	Deletes an existing network reservation bandwidth
*
*	@param[in] topic_id	The ID of the topic. Must be greater than 0
*	@param[in] topic_addr	The topic network address. Must not be a NULL pointer
*	@param[in] req_load	The ammount of reserved bandwidth ammount. Must be greater than 0
*
*	@pre			assert( topic_id );
*	@pre			assert( topic_addr );
*	@pre			assert( req_load );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*
*	@todo			Create missing error codes
*/
int tc_client_reserv_del( unsigned int topic_id, NET_ADDR *topic_addr, unsigned int req_load );

#endif
