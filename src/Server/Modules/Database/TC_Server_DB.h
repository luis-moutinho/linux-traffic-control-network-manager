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

/**	@file TC_Server_DB.h
*	@brief Function prototype for the server database module
*
*	This file contains the implementation of the functions for the server database
*	module. This module stores and manages topic and node entries in a local database. Internal module
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef TCSERVERDB_H
#define TCSERVERDB_H

#include "TC_Data_Types.h"
#include "TC_Config.h"

typedef struct topic_entry TOPIC_ENTRY;
typedef struct node_entry NODE_ENTRY;

/**
* A struct to store a nodes network status information
*/
typedef struct node_entry{

	unsigned int node_id;		/**< The ID of the node */

	NET_ADDR address;		/**< The client node address (clients top module socket address) */

	int heartbeat;			/**< The heartbeat counter */

	unsigned int uplink_load;	/**< The nodes uplink load (in bps) */
	unsigned int downlink_load;	/**< The nodes downlink load (in bps) */
		
	struct node_entry *next;	/**< The next linked list nodes entry address */
	struct node_entry *previous;	/**< The next linked list nodes entry address */

}NODE_ENTRY;

/**
* A struct to store a nodes bind status information
*/
typedef struct node_bind_entry{

	NODE_ENTRY *node;			/**< The nodes entry address */

	char req_bind;				/**< A flag to signal a node bind request */
						/**<	\li Value = 1 -> Node requesting a bind */
						/**<	\li Value = 0 -> Node is not requesting a bind */


	char req_unbind;			/**< A flag to signal a node unbind request */
						/**<	\li Value = 1 -> Node requesting an unbind */
						/**<	\li Value = 0 -> Node is not requesting an unbind */

	char is_bound;				/**< A flag to signal a node bind status */
						/**<	\li Value = 1 -> Node bound */
						/**<	\li Value = 0 -> Node not bound */

	struct node_bind_entry *next;		/**< The next linked list nodes bind entry address */
	struct node_bind_entry *previous;	/**< The next linked list nodes bind entry address */

}NODE_BIND_ENTRY;

/**
* A server side database linked list entry to store information related to a network topic and its nodes
*/
typedef struct topic_entry{

	unsigned int topic_id;		/**< The ID of the topic */
	unsigned int topic_load;	/**< The topic load (in bps) */

	NET_ADDR address;		/**< The topics network address */
	unsigned int channel_size;	/**< Maximum size (in bytes) of the topic messages */
	unsigned int channel_period;	/**< Minimum time inverval (in ms) between consecutive topic messages */

	NODE_BIND_ENTRY *prod_list;	/**< The list of producer node entries for this topic*/
	NODE_BIND_ENTRY *cons_list;	/**< The list of consumer node entries for this topic*/

	struct topic_entry *next;	/**< The next linked list topic entry address */
	struct topic_entry *previous;	/**< The previous linked list topic entry address */

}TOPIC_ENTRY;


/*@}*//**
* @name Server database module
*//*@{*/


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
int tc_server_db_lock( void );

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
int tc_server_db_unlock( void );

/**	
*	@brief Starts the server database module
*
*	Initializes the topic and node linked list and the database mutex
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_server_db_init( void );

/**
*	@brief Closes the server database module
*
*	Deletes the database mutex and all the topic and node entries

*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_server_db_close( void );
/*@}*/


/*@}*//**
* @name Node database submodule
*//*@{*/


/**	
*	@brief Gets the address of the first node database entry
*
*	Gets the address of the first node database linked list element
*
*	@return 			Upon successful return : Address of the entry
*	@return 			Upon output error : A NULL pointer
*/
NODE_ENTRY* tc_server_db_node_get_first( void );

/**
*	@brief Creates a new node entry
*
*	Creates a new entry for the node. If an entry already exists it returns that entry address instead
*
*	@param[in] node_id	The ID of the node for which the entry will be created. Must be equal or greater than 0 
*
*	@pre			assert( node_id );
*
*	@return			Upon successful return : Address of the new/existing entry
*	@return			Upon output error : A NULL pointer
*/
NODE_ENTRY* tc_server_db_node_create( unsigned int node_id );

/**
*	@brief Searches the database for the node entry
*
*	@param[in] node_id	The ID of the node to search for. Must be greater than 0
*
*	@pre			assert( node_id );
*
*	@return			Upon successful return : The node entry address
*	@return			Upon output error : A NULL pointer
*/
NODE_ENTRY* tc_server_db_node_search( unsigned int node_id );

/**
*	@brief Deletes the node entry
*
*	Removes the node entry from the linked list and frees the memory
*
*	@param[in] node		The address of the node entry to be removed. Must not be a NULL pointer
*
*	@pre			assert( node );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_server_db_node_delete( NODE_ENTRY *node );

/**
*	@brief Prints the node database
*
*	Prints all existing node entries and their properties into stdout
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_server_db_node_print ( void );

/**
*	@brief Prints the nodes entry
*
*	Prints the nodes entry properties into stdout
*
*	@param[in] entry	The address of the nodes entry. Must not be a NULL pointer
*
*	@pre			assert( entry );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_server_db_node_print_entry( NODE_ENTRY *entry );
/*@}*/


/*@}*//**
* @name Topic database submodule
*//*@{*/


/**	
*	@brief Gets the address of the first topic database entry
*
*	Gets the address of the first topic database linked list element
*
*	@return 			Upon successful return : Address of the entry
*	@return 			Upon output error : A NULL pointer
*/
TOPIC_ENTRY* tc_server_db_topic_get_first( void );

/**
*	@brief Creates a new topic entry
*
*	Creates a new entry for the topic. If an entry already exists it returns that entry address instead
*
*	@param[in] topic_id	The ID of the topic for which the entry will be created. Must be equal or greater than 0 
*
*	@pre			assert( topic_id );
*
*	@return			Upon successful return : Address of the new/existing entry
*	@return			Upon output error : A NULL pointer
*/
TOPIC_ENTRY* tc_server_db_topic_create( unsigned int topic_id );

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
TOPIC_ENTRY* tc_server_db_topic_search( unsigned int topic_id );

/**
*	@brief Deletes the topic entry
*
*	Removes the topic entry from the linked list and frees the memory
*
*	@param[in] topic	The address of the topic entry to be removed. Must not be a NULL pointer
*
*	@pre			assert( topic );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_server_db_topic_delete( TOPIC_ENTRY *topic );

/**
*	@brief Prints the topic database
*
*	Prints all existing topic entries and their properties into stdout
*
*	@pre			None
*
*	@return			Upon successful return : ERR_OK (0(
*	@return			Upon output error : An error code (<0)
*/
int tc_server_db_topic_print( void );

/**
*	@brief Prints the topics entry
*
*	Prints the topics entry properties into stdout
*
*	@param[in] entry	The address of the topic entry. Must not be a NULL pointer
*
*	@pre			assert( entry );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_server_db_topic_print_entry( TOPIC_ENTRY *entry );

/**
*	@brief Finds a node in the topics producer list
*
*	Searches for a node in the topics producer list. If found returns its node entry address
*
*	@param[in] topic	The address of the topic entry in which producers list is to search. Must not be a NULL pointer
*	@param[in] node_id	The ID of the node to search. Must be greater than 0
*
*	@pre			assert( topic );
*	@pre			assert( node_id );
*
*	@return			Upon successful return : The found nodes entry address
*	@return			Upon output error : A NULL pointer
*/
NODE_ENTRY* tc_server_db_topic_find_prod_node( TOPIC_ENTRY *topic, unsigned int node_id );

/**
*	@brief Finds a node in the topics consumer list
*
*	Searches for a node in the topics consumer list. If found returns its node entry address
*
*	@param[in] topic	The address of the topic entry in which consumers list is to search. Must not be a NULL pointer
*	@param[in] node_id	The ID of the node to search. Must be greater than 0
*
*	@pre			assert( topic );
*	@pre			assert( node_id );
*
*	@return			Upon successful return : The found nodes entry address
*	@return			Upon output error : A NULL pointer
*/
NODE_ENTRY* tc_server_db_topic_find_cons_node( TOPIC_ENTRY *topic, unsigned int node_id );

/**
*	@brief Add a node into the topics producer list
*
*	Adds a node entry in the topics producer list
*
*	@param[in] topic	The address of the topic entry in which producers list is to add. Must not be a NULL pointer
*	@param[in] node		The node entry to be added. Must not be a NULL pointer
*
*	@pre			assert( topic );
*	@pre			assert( node );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_server_db_topic_add_prod_node( TOPIC_ENTRY *topic, NODE_ENTRY *node );

/**
*	@brief Remove a node from the topics producer list
*
*	Removes a node entry from the topics producer list
*
*	@param[in] topic	The address of the topic entry in which producers list is to remove. Must not be a NULL pointer
*	@param[in] node		The node entry to be removed. Must not be a NULL pointer
*
*	@pre			assert( topic );
*	@pre			assert( node );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_server_db_topic_rm_prod_node( TOPIC_ENTRY *topic, NODE_ENTRY *node );

/**
*	@brief Add a node to the topics consumer list
*
*	Adds a node entry to the topics consumer list
*
*	@param[in] topic	The address of the topic entry in which consumers list is to add. Must not be a NULL pointer
*	@param[in] node		The node entry to be added. Must not be a NULL pointer
*
*	@pre			assert( topic );
*	@pre			assert( node );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_server_db_topic_add_cons_node( TOPIC_ENTRY *topic, NODE_ENTRY *node );

/**
*	@brief Remove a node from the topics consumer list
*
*	Removes a node entry from the topics consumer list
*
*	@param[in] topic	The address of the topic entry in which consumers list is to remove. Must not be a NULL pointer
*	@param[in] node		The node entry to be removed. Must not be a NULL pointer
*
*	@pre			assert( topic );
*	@pre			assert( node );
*
*	@return			Upon successful return : ERR_OK (0)
*	@return			Upon output error : An error code (<0)
*/
int tc_server_db_topic_rm_cons_node( TOPIC_ENTRY *topic, NODE_ENTRY *node );

/**
*	@brief Gets the number of topics consumer nodes
*
*	Counts the number of node entries existing in the topics consumer list
*
*	@param[in] topic	The address of the topic entry. Must not be a NULL pointer
*
*	@pre			assert( topic );
*
*	@return			Upon successful return : The number of consumer nodes
*	@return			Upon output error : An error code (<0)
*/
int tc_server_db_topic_number_cons_nodes( TOPIC_ENTRY *topic );

/**
*	@brief Gets the number of topics producer nodes
*
*	Counts the number of node entries existing in the topics producer list
*
*	@param[in] topic	The address of the topic entry. Must not be a NULL pointer
*
*	@pre			assert( topic );
*
*	@return			Upon successful return : The number of producer nodes
*	@return			Upon output error : An error code (<0)
*/
int tc_server_db_topic_number_prod_nodes( TOPIC_ENTRY *topic );
/*@}*/

#endif
