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

/**	@file TC_Client_DB.h
*	@brief Function prototypes for the client database module
*
*	This file contains the prototypes of the functions for the client database
*	module. This module stores and manages topic entries (with topic information) in a local database. Internal module
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef TCCLIENTDB_H
#define TCCLIENTDB_H

#include "TC_Data_Types.h"

/**
* A client side database linked list entry to store information related to a network topic
*/
typedef struct topic_c_entry{

/*@}*//**
* @name Topic Properties
*//*@{*/
	unsigned int topic_id;			/**< The ID of the topic */
	unsigned int channel_size;		/**< The topic messages maximum size in bytes */
	unsigned int channel_period;		/**< The topic messages minimum period in ms */
/*@}*/
	
/*@}*//**
* @name Topic Network Properties
*//*@{*/
	SOCK_ENTITY topic_sock;			/**< The topic associated socket entity */
	NET_ADDR topic_addr;			/**< The topic associated group address	*/
/*@}*/

/*@}*//**
* @name Topic status
*//*@{*/
	char is_updating;			/**< Flag to signal if topic is being updated */
						/**<	\li Value = 1 -> Topic updating */
						/**<	\li Value = 0 -> Topic not updating */
	
	char is_closing;			/**< Flag to signal if topic is being closed */
						/**<	\li Value = 1 -> Topic closing */
						/**<	\li Value = 0 -> Topic not closing */

	char is_consumer;			/**< Flag to signal if node is registered as consumer of this topic */
						/**<	\li Value = 1 -> Registered */
						/**<	\li Value = 0 -> Not registered */

	char is_rx_bound;			/**< Flag to signal if node is bound as consumer of this topic */
						/**<	\li Value = 1 -> Bound */
						/**<	\li Value = 0 -> Not bound */
	
	char is_producer;			/**< Flag to signal if node is registered as producer of this topic */
						/**<	\li Value = 1 -> Registered */
						/**<	\li Value = 0 -> Not registered */

	char is_tx_bound;			/**< Flag to signal if node is bound as producer of this topic */
						/**<	\li Value = 1 -> Bound */
						/**<	\li Value = 0 -> Not bound */
/*@}*/

/*@}*//**
* @name Topic Communications Control
*//*@{*/
	pthread_mutex_t topic_rx_lock;		/**< Mutex to avoid different threads receiving data at the same time */
	pthread_mutex_t topic_tx_lock;		/**< Mutex to avoid different threads sending data at the same time */
	SOCK_ENTITY unblock_rx_sock;		/**< Auxiliary socket to unblock blocked receive calls when an unbind/unregister operation is being issued */
/*@}*/	

/*@}*//**
* @name Linked List Control
*//*@{*/
	struct topic_c_entry *next;		/**< The next linked list topic entry address */
	struct topic_c_entry *previous;		/**< The previous linked list topic entry address */
/*@}*/

}TOPIC_C_ENTRY;

/**	
*	@brief Starts the client database module
*
*	Initializes the topic linked list and mutexes
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_db_init( void );

/**
*	@brief Closes the client database module
*
*	Deletes all the topic entries, topic sockets (if still active) and mutexes. Unblocks blocked receive calls
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*
*	@note			This call should not be used as a mean to unblock calls and close sockets. It is advised to use the proper unbind/unregister calls
*/
int tc_client_db_close( void );

/**
*	@brief Requests access to the database
*
*	Tries to enter the database by locking on a mutex
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*
*	@todo			Implement missing error codes
*/
int tc_client_db_lock( void );


/**
*	@brief Releases access to the database
*
*	Unlocks the database mutex
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*
*	@todo			Implement missing error codes
*/
int tc_client_db_unlock( void );

/**
*	@brief Searches the database for the topic entry
*
*	@param[in] topic_id	The ID of the topic to search for. Must be greater than 0
*
*	@pre			assert( topic_id );
*
*	@return			Upon successful return : The topic entry address
*	@return			Upon output error : A NULL pointer
*/
TOPIC_C_ENTRY* tc_client_db_topic_search( unsigned int topic_id );

/**
*	@brief Deletes the topic entry
*
*	Removes the topic entry from the linked list and frees the memory. Closes topic sockets (if active) and unblocks blocked receiving calls 
*
*	@param[in] topic	The address of the topic entry to be removed. Must not be a NULL pointer
*
*	@pre			assert( topic );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int  tc_client_db_topic_delete( TOPIC_C_ENTRY *topic );

/**
*	@brief Creates a new topic entry
*
*	Creates a new entry and the associated mutexes for the topic. If an entry already exists it returns that entry address instead
*
*	@param[in] topic_id	The ID of the topic for which the entry will be created. Must be equal or greater than 0 
*
*	@pre			assert( topic_id );
*
*	@return			Upon successful return : Address of the new/existing entry
*	@return			Upon output error : A NULL pointer
*/
TOPIC_C_ENTRY* tc_client_db_topic_create( unsigned int topic_id );

/**
*	@brief Prints all the existing entries in the database
*
*	Prints all the topic entries in the database (and their properties) to stdout
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_client_db_topic_print( void );

/**
*	@brief Gets access to the topic transmission mutex
*
*	Tries to lock the topic transmission mutex. This is used in the tc_client_topic_send( unsigned int topic_id , char *data, int data_size ) call
*	to prevent several threads to send data at the same time
*
*	@param[in] topic	The address of the topic entry (topic through where we want to send data). Must not be a NULL pointer
*	@param[in] timeout	Maximum time interval (in ms) to wait for lock on the mutex. If 0 blocks indefinitely. Must be equal or greater than 0
*
*	@pre			assert( topic );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*
*	@todo			Implement missing error codes
*/
int tc_client_lock_topic_tx( TOPIC_C_ENTRY *topic, unsigned int timeout );

/**
*	@brief Releases access to the topic transmission mutex
*
*	Unlocks the topic transmission mutex. This is used in the tc_client_topic_send( unsigned int topic_id , char *data, int data_size ) call
*	to prevent several threads to send data at the same time
*
*	@param[in] topic	The address of the topic entry (topic through where we want to send data). Must not be a NULL pointer
*
*	@pre			assert( topic );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*
*	@todo			Implement missing error codes
*/
int tc_client_unlock_topic_tx( TOPIC_C_ENTRY *topic );

/**
*	@brief Gets access to the topic reception mutex
*
*	Tries to lock the topic reception mutex. This is used in the tc_client_topic_receive( unsigned int topic_id, unsigned int timeout, char *ret_data ) call
*	to prevent several threads from receiving data at the same time
*
*	@param[in] topic	The address of the topic entry (topic from where we want to receive data). Must not be a NULL pointer
*	@param[in] timeout	Maximum time interval (in ms) to wait for lock on the mutex. If 0 blocks indefinitely. Must be equal or greater than 0
*
*	@pre			assert( topic );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*
*	@todo			Implement missing error codes
*/
int tc_client_lock_topic_rx( TOPIC_C_ENTRY *topic, unsigned int timeout );

/**
*	@brief Releases access to the topic reception mutex
*
*	Unlocks the topic reception mutex. This is used in the tc_client_topic_receive( unsigned int topic_id, unsigned int timeout, char *ret_data ) call
*	to prevent several threads from receiving data at the same time
*
*	@param[in] topic	The address of the topic entry (topic from where we want to receive data). Must not be a NULL pointer
*
*	@pre			assert( topic );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*
*	@todo			Implement missing error codes
*/
int tc_client_unlock_topic_rx( TOPIC_C_ENTRY *topic );

#endif
