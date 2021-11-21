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

/**	@file TC_Client.h
*	@brief Function prototypes for the client side API
*
*	This file contains the prototypes for the client functions
*	to be used by the application. This module also initializes all the necessary internal control modules. Top module
*  
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef TCCLIENT_H
#define TCCLIENT_H

//Event codes (to separate this layer from tc_data_types)

/**	@def NODE_PLUG
*	@brief Code for node registration event.
*/
#define NODE_PLUG	1
/** 	@def NODE_UNPLUG
*	@brief Code for node unregistration event.
*/
#define NODE_UNPLUG	0

/**	
*	@brief Starts the client module
*
*	Initializes all the client required modules and searches for an active server in the network or in the same local node.
*	Upon discovery it will send a node registration request to the server and wait for the reply
*
*	@param[in] ifface	The NIC interface to be used to connect to the network. Must not be a NULL pointer
*	@param[in] node_id 	The desired node ID to be registered with. If 0 server assigns a random ID. Must be equal or greater than 0
*
*	@pre			None
*
*	@return 		Upon successful return : The assigned node ID
*	@return 		Upon output error : An error code (<0)
*/
int tc_client_init( char *ifface, unsigned int node_id );

/**	
*	@brief Closes the client module
*
*	Stops all active comunications and deregisters the node from the network. Closes all the client modules.
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_close( void );

/**
*	@brief Polls for an event trigger message
*
*	Waits until an event occurs and server sends an event trigger message ( to the client notifications module ).
*	In this current version only nodes registration and deregistration events are triggered
*
*	@param[in] timeout 	The maximum time interval (in ms) to wait for an event. If 0 waits for an event indefinitely. Must be equal or greater than 0
*	@param[out] ret_event	The buffer where to store the event code. Must not be a NULL pointer
*	@param[out] ret_node_id The buffer where to store the ID of the node who triggered the event. Must not be a NULL pointer
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
int tc_client_get_node_event( unsigned int timeout, unsigned char *ret_event, unsigned int *ret_node_id );

/**
*	@brief Registers a topic in the network
*
*	Sends a request to the server for the registration of a new network topic.
*	If a topic with the same ID already exists and with different properties this request is refused.
*
*	@param[in] topic_id	The desired ID for the topic. Must be greater than 0
*	@param[in] size		The maximum size (in bytes ) of the messages to be sent through this topic. Must be greater than 0
*	@param[in] period	The mininum time interval (in ms) between consecutive topic messages. Must be greater than 0
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_topic_create( unsigned int topic_id, unsigned int size, unsigned int period );

/**
*	@brief Destroys the network topic
*
*	Sends a request to the server for the destruction of the network topic.
*	All the comunications on that topic are closed. All the registered/bound nodes will unregister/unbind themselves and all the associated topic reservations are freed.
*	The topic entry in the nodes database is also deleted
*
*	@param[in] topic_id	The ID of the topic to be destroyed. Must be greater than 0
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_topic_destroy( unsigned int topic_id );

/**
*	@brief Retrieves existing topic properties
*
*	Sends a request to the server for the retrieval of the topic properties.
*	If the topic does not exist the request is refused
*
*	@param[in] topic_id	The ID of the topic to get the properties from. Must be greater than 0
*	@param[out] ret_size	The buffer where to store the maximum size (in bytes) of the topic messages. Optional parameter, can be a NULL pointer
*	@param[out] ret_period	The buffer where to store the minimum interval (in ms) between consecutive topic messages. Optional parameter, can be a NULL pointer
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_topic_get_prop( unsigned int topic_id , unsigned int *ret_size, unsigned int *ret_period );

/**
*	@brief Sets topic with new properties
*
*	Sends a request to the server to update the topic properties.
*	Server will check if there is enough resources in all nodes for the requested changes.
*	If there is enough resources, the topic database entry and reservation are updated in all the associated nodes
*
*	@param[in] topic_id	The ID of the topic to set the new properties. Must be greater than 0
*	@param[in] new_size	The new maximum size (in bytes) of the topic messages. Must be greater than 0
*	@param[in] new_period	The new minimum interval (in ms) between consecutive topic messages. Must be greater than 0
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_topic_set_prop( unsigned int topic_id , unsigned int new_size, unsigned int new_period );

/**
*	@brief Registers client as producer of topic
*
*	Sends a request to the server to register the node as producer of the network topic.
*	Server will check if there is enough resources on all the nodes associated to this topic. If there is the registration is accepted.
*	A resource reservation associated with the topic is created on the requesting node
*
*	@param[in] topic_id	The ID of the topic to be registered to. Must be greater than 0
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*
*	@note			Client can't send data through this topic after this call. It needs to also bind himself to the topic as producer with the call tc_client_bind_tx( unsigned int topic_id, unsigned int timeout )
*/
int tc_client_register_tx( unsigned int topic_id );

/**
*	@brief Unregisters client as producer of topic
*
*	Sends a request to the server to unregister the node as producer of the network topic.
*	If the node is also bound it will unbind and stop the comunications on that topic as producer.
*	The resource reservation associated with the topic is freed on the requesting node.
*	After this server will check all the nodes associated with this topic and unbind all the consumers if there is no valid producer for them
*
*	@param[in] topic_id	The ID of the topic to be unregistered from. Must be greater than 0
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_unregister_tx( unsigned int topic_id );

/**
*	@brief Registers client as consumer of topic
*
*	Sends a request to the server to register the node as consumer of the network topic.
*	Server will check if there is enough resources on all the nodes associated to this topic. If there is the registration is accepted.
*	No resource reservation associated with the topic is created on the requesting node in this current version (we only control the uplinks)
*
*	@param[in] topic_id	The ID of the topic to be registered to. Must be greater than 0
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*
*	@note			Client can't receive data from this topic after this call. It needs to also bind himself to the topic as consumer with the call tc_client_bind_rx( unsigned int topic_id, unsigned int timeout )
*/
int tc_client_register_rx( unsigned int topic_id );

/**
*	@brief Unregisters client as consumer of topic
*
*	Sends a request to the server to unregister the node as consumer of the network topic.
*	If the node is also bound it will unbind and stop the comunications on that topic as consumer.
*	After this server will check all the nodes associated with this topic and unbind all the producers if there is no valid consumer for them
*
*	@param[in] topic_id	The ID of the topic to be unregistered from. Must be greater than 0
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_unregister_rx( unsigned int topic_id );

/**
*	@brief Binds client as producer of topic
*
*	Sends a request to the server to bind the node as producer of the network topic.
*	Upon receiving a positive reply from the server the client will wait until it is bound.
*	The server will bind the nodes when there is at least one valid producer and one consumer waiting for bind or already bound
*
*	@param[in] topic_id	The ID of the topic to be bound to. Must be greater than 0
*	@param[in] timeout	Maximum time interval (in ms) to wait for the node to be bound. If 0 blocks indefinitely. Must be equal or greater than 0
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*
*	@note 			The node had to be previously registered as producer of the topic
*/
int tc_client_bind_tx( unsigned int topic_id, unsigned int timeout );

/**
*	@brief Unbinds client as producer of topic
*
*	Sends a request to the server to unbind the node as producer of the network topic.
*	The node will unbind and stop the comunications on that topic as producer.
*	After this server will check all the nodes associated with this topic and unbind all the consumers if there is no valid producer for them
*
*	@param[in] topic_id	The ID of the topic to be unbound from. Must be greater than 0
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*
*	@note 			The reservation associated with this topic is not freed with this call and the node is still registered as producer
*/
int tc_client_unbind_tx( unsigned int topic_id );

/**
*	@brief Binds client as consumer of topic
*
*	Sends a request to the server to bind the node as consumer of the network topic.
*	Upon receiving a positive reply from the server the client will wait until it is bound.
*	The server will bind the nodes when there is at least one valid producer and one consumer waiting for bind or already bound
*
*	@param[in] topic_id	The ID of the topic to be bound to. Must be greater than 0
*	@param[in] timeout	Maximum time interval (in ms) to wait for the node to be bound. If 0 blocks indefinitely. Must be equal or greater than 0
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*
*	@note			The node had to be previously registered as consumer of the topic
*/
int tc_client_bind_rx( unsigned int topic_id, unsigned int timeout );

/**
*	@brief Unbinds client as consumer of topic
*
*	Sends a request to the server to unbind the node as consumer of the network topic.
*	The node will unbind and stop the comunications on that topic as consumer.
*	After this server will check all the nodes associated with this topic and unbind all the producers if there is no valid consumer for them
*
*	@param[in] topic_id	The ID of the topic to be unbound from. Must be greater than 0
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_unbind_rx( unsigned int topic_id );

/**
*	@brief Sends a message through the network topic
*
*	Sends a message through the network topic. If the size of the message exceeds the maximum size of the topic messages an error is triggered
*
*	@param[in] topic_id	The ID of the topic through which the message is to be sent. Must be greater than 0
*	@param[in] data		The buffer with the data of the sending message. Must not be a NULL pointer
*	@param[in] data_size	The size of the sending message. Must be greater than 0
*
*	@return			Upon successful return : The number of sent bytes
*	@return			Upon output error : An error code (<0)
*
*	@note 			Client must be registered and bound as producer to be able to use this call
*/
int tc_client_topic_send( unsigned int topic_id , char *data, int data_size );

/**
*	@brief Receives a message from the network topic
*
*	Receives a message from the network topic
*
*	@param[in] topic_id	The ID of the topic from which the message is to be received. Must be greater than 0
*	@param[in] timeout	The maximum time (in ms) to wait for a message. If 0 blocks indefinitely. Must be equal or greater than 0
*	@param[out] ret_data	The buffer where to store the received message. Must not be a NULL pointer
*
*	@return			Upon successful return : The number of received bytes
*	@return			Upon output error : An error code (<0)
*
*	@note			Client must be registered and bound as consumer to be able to use this call
*
*	@todo			Handle message fragmentation with topic messages which size exceeds D_MTU :
*					- Reception of fragments of different messages from different producers when one or more producers are transmiting at the same time
*					- Message fragments can be lost when client tries to send data faster than topic period	
*/
int tc_client_topic_receive( unsigned int topic_id, unsigned int timeout, char *ret_data );

#endif
