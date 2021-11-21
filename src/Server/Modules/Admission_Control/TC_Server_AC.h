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

/**	@file TC_Server_AC.h
*	@brief Function prototypes for the server admission control module
*
*	This file contains the prototypes of the functions for the server admission control module. 
*	This module receives the requests from the client and accepts or refuses them. Uses other internal modules
*	to carry out the necessary operations in order to fulfill the accepted requests.
*	Internal module
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef TCSERVERAC_H
#define TCSERVERAC_H

#include "Sockets.h"

/**	
*	@brief Starts the server admission control module
*
*	Initializes the module	
*
*	@pre				Node
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*/
int tc_server_ac_init( void );

/**	
*	@brief Closes the server admission control module
*
*	Closes the module	
*
*	@pre				Node
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*/
int tc_server_ac_close( void );

/**	
*	@brief Registers a node in the network
*
*	Checks if node is already registered. If not creates a new entry in the database and assigns an ID for the node	
*
*	@param[in] node_id		The ID requested by the node to be registered with. If 0 server assigns a random ID. Must be equal or greater than 0
*	@param[in] node_address		The nodes network address (clients top module socket address). Must not be a NULL pointer
*	@param[out] ret_node_id		The assigned ID to the node. Must not be a NULL pointer
*
*	@pre				assert( node_address ); 
*	@pre				assert( ret_node_id ); 
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*
*	@todo				In this version it is only supported the registration of one local client. Redo the function
*					so it is possible to register more than one (don't forget about the traffic control reservations!!) 
*/
int tc_server_ac_add_node( unsigned int node_id, NET_ADDR *node_address, unsigned int *ret_node_id );

/**	
*	@brief Removes a node from the network
*
*	Check if node is registered. If it is the management module function tc_server_management_rm_node( NODE_ENTRY *node ) is called to remove the node
*
*	@param[in] node_id		The ID of the node to remove. Must be greater than 0
*
*	@pre				assert( node_id ); 
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*/
int tc_server_ac_rm_node( unsigned int node_id );

/**	
*	@brief Registers a new topic in the network
*
*	Checks if topic is already registered. If it is already registered but with different properties the request is refused otherwise it is accepted.
*	If it is not registered a new entry is created and a network address is assigned to the topic	
*
*	@param[in] topic_id		The ID requested by the node to be registered with. If 0 server assigns a random ID. Must be equal or greater than 0
*	@param[in] channel_size		The nodes network address (clients top module socket address). Must not be a NULL pointer
*	@param[in] channel_period	The assigned ID to the node. Must not be a NULL pointer
*
*	@pre				assert( topic_id ); 
*	@pre				assert( channel_size ); 
*	@pre				assert( channel_period ); 
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*
*	@todo				In this version it is supported a high but limited number of addresses for the topics. Find a better
*					way to generate addresses or an address pool for store and reuse of the freed addresses
*/
int tc_server_ac_add_topic( unsigned int topic_id, unsigned int channel_size, unsigned int channel_period );

/**	
*	@brief Updates the topic properties
*
*	Calculates the new load of the topic after the requested changes and checks if every node has enough bandwidth. If they have it calls
*	the management module to update the reservations and databases of all the nodes otherwise the request is refused
*
*	@param[in] topic_id		The ID of the topic to update. Must be greater than 0
*	@param[in] channel_size		The new maximum size (in bytes) for the topic messages. Must be greater than 0
*	@param[in] channel_period	The minimum time interval (in ms) between consecutive topic messages. Must be greater than 0
*
*	@pre				assert( topic_id ); 
*	@pre				assert( channel_size );
*	@pre				assert( channel_period );
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*/
int tc_server_ac_set_topic_prop( unsigned int topic_id, unsigned int channel_size, unsigned int channel_period );

/**	
*	@brief Removes a topic from the network
*
*	Checks if topic is registered. If it is the management module function tc_server_management_rm_topic( TOPIC_ENTRY *topic ) is called to remove the topic
*	from all the nodes. Updates nodes bandwidth after topic removal
*
*	@param[in] topic_id		The ID of the topic to remove. Must be greater than 0
*
*	@pre				assert( topic_id ); 
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*/
int tc_server_ac_rm_topic( unsigned int topic_id );

/**	
*	@brief Retrieves the topic properties
*
*	Searches for the topic entry and gets its properties. If topic is not found the request is refused
*
*	@param[in] topic_id		The ID of the topic. Must be greater than 0
*	@param[out] ret_load		The buffer to store the topics load (in bps). Optional (can be a NULL pointer)
*	@param[out] ret_size		The buffer to store the maximum size (in bytes) of the topic messages. Optional (can be a NULL pointer)
*	@param[out] ret_period		The buffer to store the minimum time interval (in ms) between consecutive messages. Optional (can be a NULL pointer)
*	@param[out] ret_topic_addr	The buffer to store the topics network address. Optional (can be a NULL pointer)
*
*	@pre				assert( topic_id );
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*/
int tc_server_ac_get_topic_prop( unsigned int topic_id, unsigned int *ret_load, unsigned int *ret_size, unsigned int *ret_period, NET_ADDR *ret_topic_addr );

/**	
*	@brief Registers a node as a topic producer
*
*	Registers a node as a topic producer. If the node or topic is not registered in the database the request is refused.
*	Checks if all the nodes have enough bandwidth for a new producer. If they have enough bandwidth the management module is called to
*	request a reservation in the producer. After the reservation is made the bandwidth of all nodes is updated and the node entry is
*	added to the topics producer list
*
*	@param[in] topic_id		The ID of the topic. Must be greater than 0
*	@param[in] node_id		The ID of the node. Must be greater than 0
*
*	@pre				assert( topic_id );
*	@pre				assert( node_id );  
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*/
int tc_server_ac_add_prod( unsigned int topic_id, unsigned int node_id );

/**	
*	@brief Unregisters a node as a topic producer
*
*	Unregisters a node as a topic producer. If the node or topic is not registered in the database the request is refused.
*	The management module is called to free the reservation in the producer. After the reservation is freed the bandwidth of all nodes is updated
*	and the node entry removed from the topic producers list 
*
*	@param[in] topic_id		The ID of the topic. Must be greater than 0
*	@param[in] node_id		The ID of the node. Must be greater than 0
*
*	@pre				assert( topic_id );
*	@pre				assert( node_id );  
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*
*	@note				The server top module will call tc_server_ac_check_topic_unbind( unsigned int topic_id ) after this request has been resolved
*/
int tc_server_ac_rm_prod( unsigned int topic_id, unsigned int node_id );

/**	
*	@brief Registers a node as a topic consumer
*
*	Registers a node as a topic consumer. If the node or topic is not registered in the database the request is refused.
*	Checks if all the nodes have enough bandwidth for a new consumer. If they have enough bandwidth the node entry is added to the topics
*	consumer list and the bandwidth of all nodes updated
*
*	@param[in] topic_id		The ID of the topic. Must be greater than 0
*	@param[in] node_id		The ID of the node. Must be greater than 0
*
*	@pre				assert( topic_id );
*	@pre				assert( node_id );  
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*
*	@note				Since we use multicast only the requesting node bandwidth will be affected and no new reservations are made
*/
int tc_server_ac_add_cons( unsigned int topic_id, unsigned int node_id );

/**	
*	@brief Unregisters a node as a topic consumer
*
*	Unregisters a node as a topic consumer. If the node or topic is not registered in the database the request is refused.
*	The node is removed from the topics consumer list and the bandwidth of all nodes is updated	
*
*	@param[in] topic_id		The ID of the topic. Must be greater than 0
*	@param[in] node_id		The ID of the node. Must be greater than 0
*
*	@pre				assert( topic_id );
*	@pre				assert( node_id );  
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*
*	@note				The server top module will call tc_server_ac_check_topic_unbind( unsigned int topic_id ) after this request has been resolved
*/
int tc_server_ac_rm_cons( unsigned int topic_id, unsigned int node_id );

/**	
*	@brief Binds a node as a topic producer
*
*	Binds a node as a topic producer. If the node or topic is not registered in the database the request is refused.
*	The nodes bind status entry is updated signaling that the node is requesting a bind 
*
*	@param[in] topic_id		The ID of the topic. Must be greater than 0
*	@param[in] node_id		The ID of the node. Must be greater than 0
*
*	@pre				assert( topic_id );
*	@pre				assert( node_id );  
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*
*	@note				The server top module will call tc_server_ac_check_topic_bind( unsigned int topic_id ) after this request has been resolved
*/
int tc_server_ac_bind_tx( unsigned int topic_id, unsigned int node_id );

/**	
*	@brief Unbinds a node as a topic producer
*
*	Unbinds a node as a topic producer. If the node or topic is not registered in the database the request is refused.
*	The nodes bind status entry is updated signaling that the node is requesting an unbind 
*
*	@param[in] topic_id		The ID of the topic. Must be greater than 0
*	@param[in] node_id		The ID of the node. Must be greater than 0
*
*	@pre				assert( topic_id );
*	@pre				assert( node_id );  
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*
*	@note				The server top module will call tc_server_ac_check_topic_unbind( unsigned int topic_id ) after this request has been resolved
*/
int tc_server_ac_unbind_tx( unsigned int topic_id, unsigned int node_id );

/**	
*	@brief Binds a node as a topic consumer
*
*	Binds a node as a topic consumer. If the node or topic is not registered in the database the request is refused.
*	The nodes bind status entry is updated signaling that the node is requesting a bind 
*
*	@param[in] topic_id		The ID of the topic. Must be greater than 0
*	@param[in] node_id		The ID of the node. Must be greater than 0
*
*	@pre				assert( topic_id );
*	@pre				assert( node_id );  
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*
*	@note				The server top module will call tc_server_ac_check_topic_bind( unsigned int topic_id ) after this request has been resolved
*/
int tc_server_ac_bind_rx( unsigned int topic_id, unsigned int node_id );

/**	
*	@brief Unbinds a node as a topic consumer
*
*	Unbinds a node as a topic consumer. If the node or topic is not registered in the database the request is refused.
*	The nodes bind status entry is updated signaling that the node is requesting an unbind 
*
*	@param[in] topic_id		The ID of the topic. Must be greater than 0
*	@param[in] node_id		The ID of the node. Must be greater than 0
*
*	@pre				assert( topic_id );
*	@pre				assert( node_id );  
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*
*	@note				The server top module will call tc_server_ac_check_topic_unbind( unsigned int topic_id ) after this request has been resolved
*/
int tc_server_ac_unbind_rx( unsigned int topic_id, unsigned int node_id );

/**	
*	@brief Checks for nodes with pending bind requests
*
*	Calls the management module to check for nodes with pending bind requests and to bind all that have a valid producer/consumer partner
*
*	@param[in] topic_id		The ID of the topic. Must be greater than 0
*
*	@pre				assert( topic_id );
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*/
int tc_server_ac_check_topic_bind( unsigned int topic_id );

/**	
*	@brief Checks for nodes with pending unbind requests
*
*	Calls the management module to check for nodes with pending unbind requests and to unbind all that does not have a valid producer/consumer partner or
*	is requesting an unbind
*
*	@param[in] topic_id		The ID of the topic. Must be greater than 0
*
*	@pre				assert( topic_id );
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*/
int tc_server_ac_check_topic_unbind( unsigned int topic_id );

#endif
