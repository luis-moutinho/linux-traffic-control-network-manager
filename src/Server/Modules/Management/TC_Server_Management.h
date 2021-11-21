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

/**	@file TC_Server_Management.h
*	@brief Function prototype for the server management module
*
*	This file contains the prototype of the functions for the server management module. 
*	This module is an auxiliary module for the servers admission control module. This module issues the necessary
*	control operations and the necessary requests to the clients for the node removal,topic update and bind/unbind check routines.
*	Internal module
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef TCSERVERMANAGEMENT_H
#define TCSERVERMANAGEMENT_H

#include "TC_Server_DB.h"

/**	
*	@brief Starts the server management module
*
*	Initializes the module and creates the necessary sockets to comunicate with the clients management modules.
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
int tc_server_management_init( NET_ADDR *server_remote );

/**	
*	@brief Closes the server management module
*
*	Closes the module and sockets
*
*	@pre				None
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*/
int tc_server_management_close( void );

/**	
*	@brief Sends a reservation request to a client
*
*	Sends a resource reservation request to a clients management module
*
*	@param[in] node			The entry address of the node where the reservation is to be made. Must not be a NULL pointer
*	@param[in] topic		The entry address of the topic associated with the reservation. Must not be a NULL pointer
*	@param[in] tc_request		The reservation operation type code. Must be one of the defined codes (TC_RESERV/TC_FREE/TC_MODIFY)
*	@param[in] req_load		The ammount of load to be reserved/freed. Must be greater than 0
*
*	@pre				assert( node );
*	@pre				assert( topic );
*	@pre				assert( tc_request == TC_RESERV || tc_request == TC_FREE || tc_request == TC_MODIFY );
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*/
int tc_server_management_reserv_req( NODE_ENTRY *node, TOPIC_ENTRY *topic, unsigned char tc_request, unsigned int req_load );

/**	
*	@brief Removes a topic
*
*	Sends a request to all the nodes associated with the topic to delete it. Each node will locally unbind and unregister himself from the topic
*	while freeing the associated reservation. Nodes will then reply back if the operation was a success or a failure
*
*	@param[in] topic		The entry address of the topic associated with the reservation. Must not be a NULL pointer
*
*	@pre				assert( topic );
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*
*	@todo				Implement the correct procedure when one or more nodes fail the operation
*					(right now only an error code and list with the nodes that failed is generated)
*/
int tc_server_management_rm_topic( TOPIC_ENTRY *topic );

/**	
*	@brief Updates topic properties
*
*	Sends a request to all the nodes associated with the topic to update it with new properties. Each node will locally update the
*	associated reservation and topic entry. Nodes will then reply back if the operation was a success or a failure
*
*	@param[in] topic		The entry address of the topic associated with the reservation. Must not be a NULL pointer
*
*	@pre				assert( topic );
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*
*	@todo				Implement the correct procedure when one or more nodes fail the operation
*					(right now only an error code and list with the nodes that failed is generated)
*/
int tc_server_management_set_topic( TOPIC_ENTRY *topic );

/**	
*	@brief Removes a node
*
*	Removes a node after deregistration (called from servers admission control module) or upon dead node declaration (called from the servers monitoring module).
*	This call will update all the afected nodes bandwidth (in case the removed node was a producer of some topic). It will also call
*	tc_server_management_check_unbind( TOPIC_ENTRY *topic ) to unbind all the possible nodes that don't have valid consumers/producers anymore and remove the node entry
*	from the database
*
*	@param[in] node			The entry address of the node to be removed. Must not be a NULL pointer
*
*	@pre				assert( topic );
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*
*	@todo				Implement the correct procedure when one or more nodes fail the operation
*					(right now only an error code and list with the nodes that failed is generated)
*/
int tc_server_management_rm_node( NODE_ENTRY *node );

/**	
*	@brief Checks for possible topic binds
*
*	Checks in the topic for nodes requesting for a bind. A bind request is then sent to all the nodes that have valid
*	producer/consumer partners. Nodes will then reply back if the bind was a success or a failure. Updates the nodes
*	bind status in the database
*
*	@param[in] topic		The entry address of the topic to be checked. Must not be a NULL pointer
*
*	@pre				assert( topic );
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*
*	@todo				Implement the correct procedure when one or more nodes fail the operation
*					(right now only an error code and list with the nodes that failed is generated)
*/
int tc_server_management_check_bind( TOPIC_ENTRY *topic );

/**	
*	@brief Checks for possible topic unbinds
*
*	Checks in the topic for nodes that are bound or requesting an unbind. An unbind request is then sent to all the nodes that don't have valid
*	producer/consumer partners. Nodes will then reply back if the unbind was a success or a failure. Updates the nodes
*	bind status in the database
*
*	@param[in] topic		The entry address of the topic to be checked. Must not be a NULL pointer
*
*	@pre				assert( topic );
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*
*	@todo				Implement the correct procedure when one or more nodes fail the operation
*					(right now only an error code and list with the nodes that failed is generated)
*/
int tc_server_management_check_unbind( TOPIC_ENTRY *topic );

#endif
