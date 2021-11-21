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

/**	@file TC_Error_Types.h
*	@brief Function prototypes for the error utility module
*
*	This file contains the function prototypes for the error utility module.
*	Contains a function to analyse an error code and print the type of the error in a human reading way
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef ERRORTYPE_H
#define ERRORTYPE_H

/**
* A definition of several types of errors
*/
typedef enum {


/*@}*//**
* @name Misc errors
*//*@{*/
 

ERR_OK = 0,		/**< No error occurred */
ERR_INVALID_NIC,	/**< Invalid NIC */
ERR_DISCOVERY_SERVER,	/**< Error discovering server */
ERR_INVALID_PARAM,	/**< Invalid call parameters */
ERR_MEM_MALLOC,		/**< Error getting memory */
/*@}*/


/*@}*//**
* @name Module Related Errors
*//*@{*/


ERR_C_NOT_INIT = -1000,	/**< Client module not initialized */
ERR_C_ALREADY_INIT,	/**< Client module already initialized */
ERR_S_NOT_INIT,		/**< Server module not initialized */
ERR_S_ALREADY_INIT,	/**< Server module already initialized */

ERR_TC_INIT,		/**< Error initializing linux traffic control */
ERR_COMM_INIT,		/**< Error initializing comunications module */
ERR_DB_INIT,		/**< Error initializing database module */
ERR_MONIT_INIT,		/**< Error initializing monitoring module */
ERR_RESERV_INIT,	/**< Error initializing reservation module */
ERR_MANAG_INIT,		/**< Error initializing management module */
ERR_AC_INIT,		/**< Error initializing admission control module */
ERR_DISCOVERY_INIT,	/**< Error initializing discovery module */
ERR_NOTIFIC_INIT,	/**< Error initializing notifications module */
ERR_HANDLER_INIT,	/**< Error initializing server handler thread */

ERR_TC_CLOSE,		/**< Error closing linux traffic control */
ERR_COMM_CLOSE,		/**< Error closing comunications module */
ERR_DB_CLOSE,		/**< Error closing database module */
ERR_MONIT_CLOSE,	/**< Error closing monitoring module */
ERR_RESERV_CLOSE,	/**< Error closing reservation module */
ERR_MANAG_CLOSE,	/**< Error closing management module */
ERR_AC_CLOSE,		/**< Error closing admission control module */
ERR_DISCOVERY_CLOSE,	/**< Error closing discovery module */
ERR_NOTIFIC_CLOSE,	/**< Error closing notifications module */
ERR_HANDLER_CLOSE,	/**< Error closing server handler thread */
/*@}*/


/*@}*//**
* @name Socket Related Errors
*//*@{*/


ERR_SOCK_CREATE,	/**< Error creating a socket */
ERR_SOCK_TYPE,		/**< Invalid socket type */
ERR_SOCK_ENTITY,	/**< Invalid socket entity */
ERR_SOCK_INVALID_FD,	/**< Invalid socket file descriptor */
ERR_SOCK_OPTION,	/**< Error setting a socket option */
ERR_SOCK_BIND_HOST,	/**< Error binding socket to host address */
ERR_SOCK_BIND_PEER,	/**< Error binding socket to peer address */
ERR_SOCK_CONNECT,	/**< Error connecting to host */
ERR_SOCK_DISCONNECT,	/**< Error disconnecting from host */
ERR_SOCK_CLOSE,		/**< Error closing socket */
/*@}*/




/*@}*//**
* @name Control Messages Related Errors
*//*@{*/


ERR_SEND_REQUEST,	/**< Error sending request */
ERR_GET_REQUEST,	/**< Error receiving request */
ERR_SEND_ANSWER,	/**< Error sending answer */
ERR_GET_ANSWER,		/**< Error receiving answer */
/*@}*/


/*@}*//**
* @name Topic Messages Related Errors
*//*@{*/


ERR_DATA_INVALID,	/**< Invalid data */
ERR_DATA_TIMEOUT,	/**< Timedout while receiving/sending data */
ERR_DATA_UNBLOCK,	/**< Received unblock signal */
ERR_DATA_SEND,		/**< Error while sending data */
ERR_DATA_RECEIVE,	/**< Error while receiving data */
ERR_DATA_SIZE,		/**< Error with data size */
/*@}*/


/*@}*//**
* @name Thread Related Errors
*//*@{*/


ERR_PTHREAD_CREATE,	/**< Error creating thread */
ERR_PTHREAD_DESTROY,	/**< Error destroying thread */
ERR_THREAD_CREATE,	/**< Error while creating thread */
ERR_THREAD_DESTROY,	/**< Error while destroying thread */
ERR_THREAD_TIMEOUT,	/**< Timedout while waiting for thread to lock feedback mutex */
/*@}*/


/*@}*//**
* @name Topic Related Errors
*//*@{*/


ERR_TOPIC_NOT_REG,	/**< Topic does not exist */
ERR_TOPIC_DIFF_PROP,	/**< Topic exists with different properties */

ERR_TOPIC_CREATE,	/**< Error creating topic entry */
ERR_TOPIC_DELETE,	/**< Error destroying topic entry */
ERR_TOPIC_UPDATE,	/**< Error updating topic entry */
ERR_TOPIC_IN_UPDATE,	/**< Error occurred because topic is updating */

ERR_TOPIC_LOCAL_CREATE,/**< Error creating topic local entry */
ERR_TOPIC_LOCAL_DELETE,/**< Error destroying topic local entry */

ERR_TOPIC_JOIN_TX,	/**< Error joining topic group as producer */
ERR_TOPIC_JOIN_RX,	/**< Error joining topic group as consumer */

ERR_TOPIC_CLOSING,	/**< Topic is being closed */
/*@}*/


/*@}*//**
* @name Node Related Errors
*//*@{*/


ERR_REG_NODE,		/**< Error registering node */
ERR_NODE_DELETE,	/**< Error deleting node entry */
ERR_UNREG_NODE,		/**< Error unregistering node */
ERR_NODE_BIND,		/**< Error binding node */
ERR_NODE_UNBIND,	/**< Error unbinding node */
ERR_NODE_MAX,		/**< Max number of nodes exceeded */

ERR_NODE_DIFF_ADDR,	/**< Error node with same ID registered with different address */
ERR_NODE_NOT_REG,	/**< Node is not registered in the server */
ERR_NODE_NOT_REG_TX,	/**< Node is not registered as producer of the topic */
ERR_NODE_NOT_REG_RX,	/**< Node is not registered as consumer of the topic */

ERR_NODE_PROD_BW,	/**< Producer nodes dont have enough bandwidth  */
ERR_NODE_CONS_BW,	/**< Consumer nodes dont have enough bandwidth  */

ERR_NODE_PROD_RESERV,	/**< Bandwidth reservation failed on producer node  */
ERR_NODE_CONS_RESERV,	/**< Bandwidth reservation failed on consumer node */

ERR_NODE_PROD_F_RESERV,	/**< Bandwidth reservation failed on producer node  */
ERR_NODE_CONS_F_RESERV,	/**< Bandwidth reservation failed on consumer node */

 
ERR_NODE_PROD_REG,	/**< Node registration as producer of topic failed */
ERR_NODE_CONS_REG,	/**< Node registration as consumer of topic failed */

ERR_NODE_PROD_UNREG,	/**< Node unregistration as producer of topic failed */
ERR_NODE_CONS_UNREG,	/**< Node unregistration as consumer of topic failed */

ERR_BIND_TX_TIMEDOUT,	/**< Timedout while waiting for bind as producer on topic */
ERR_BIND_RX_TIMEDOUT,	/**< Timedout while waiting for bind as consumer on topic */
ERR_UNBIND_TX_TIMEDOUT,	/**< Timedout while waiting for unbind as producer on topic */
ERR_UNBIND_RX_TIMEDOUT,	/**< Timedout while waiting for unbind as consumer on topic */

ERR_NODE_NOT_BOUND_TX,	/**< Node is not bound as producer of the topic */
ERR_NODE_NOT_BOUND_RX,	/**< Node is not bound as consumer of the topic */
/*@}*/


/*@}*//**
* @name Reservation Related Errors
*//*@{*/


ERR_RESERV_ADD,		/**< Error while creating reservation */
ERR_RESERV_DEL,		/**< Error while destroying reservation */
ERR_RESERV_SET,		/**< Error while modifying reservation */
/*@}*/

}ERR_TYPE;


/**	
*	@brief Prints the error code
*
*	Prints a human readable string describing the kind of error from an error code
*
*	@param[in] error_code	The error code to be analysed
*
*	@pre			None
*
*	@return			Upon successful return : 0
*	@return			Upon output error : < 0
*
*	@todo			Create more error codes and organize them better
*/
int tc_error_print ( ERR_TYPE error_code );

#endif
