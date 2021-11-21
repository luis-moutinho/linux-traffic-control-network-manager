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

/**	@file TC_Utils.h
*	@brief Utilities function prototypes
*
*	This file contains the prototype of the utilities functions. These functions
*	aid in threads creation, NIC IP address retrieval and the transformation of data messages
*	to host-to-network or network-to-host format
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef TCUTILS_H
#define TCUTILS_H

#include "TC_Data_Types.h"

/**	@def CEILING(X)
*	@brief Computes the ceil operation on the var
*/
#ifndef CEILING
#define CEILING(X) (X-(int)(X) > 0 ? (int)(X+1) : (int)(X))
#endif

/**	@def THREAD_RUN
*	@brief Flag to signal thread loop to keep running
*/
#define THREAD_RUN 0

/**	@def THREAD_STOP
*	@brief Flag to signal thread loop to stop running
*/
#define THREAD_STOP 1

/**	
*	@brief Transforms and send the data message
*
*	Transforms the data message into host-to-network format and sends it through the socket	
*
*	@param[in] sock			The socket entity to send the message through. Must not be a NULL pointer
*	@param[in] msg			The message to be sent. Must not be a NULL pointer
*	@param[in] peer			The address of the destination. Optional (can be a NULL pointer when using connected sockets)
*
*	@pre				assert(sock);
*	@pre				assert(msg);
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*/
int tc_network_send_msg( SOCK_ENTITY *sock, NET_MSG *msg, NET_ADDR *peer );

/**	
*	@brief Receives and transforms the data message
*
*	Receives message from socket and transforms it into network-to-host format
*
*	@param[in] sock			The socket entity to received the message from. Must not be a NULL pointer
*	@param[in] timeout		Maximum time interval (in ms) to wait for a message. If 0 blocks indefinitely. Must be equal or greater than 0
*	@param[out] ret_msg		The buffer to store the received message. Must not be a NULL pointer
*	@param[out] ret_sender		The address of the sender. Optional (can be a NULL pointer)
*
*	@pre				assert(sock);
*	@pre				assert(ret_msg);
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*/
int tc_network_get_msg( SOCK_ENTITY *sock, unsigned int timeout, NET_MSG *ret_msg, NET_ADDR *ret_sender );

/**	
*	@brief Retrives the NICs IP
*
*	Creates the necessary socket to retrieve the NICs IP address
*
*	@param[in] ifface		The NIC name. Must not be a NULL pointer
*	@param[out] ret_ip		The buffer to store the NICs IP address. Must not be a NULL pointer
*
*	@pre				assert(ifface);
*	@pre				assert(ret_ip);
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*/
int tc_network_get_nic_ip( char *ifface , char *ret_ip );

/**	
*	@brief Creates a thread
*
*	Creates and checks if the thread was created and is running. Creates feedback mutex if requested
*
*	@param[in] thread_call		The thread code call. Must not be a NULL pointer
*	@param[out] ret_thread_id	The buffer to store the created thread ID. Must not be a NULL pointer
*	@param[out] ret_quit_flag	The address of the thread quit flag. Must not be a NULL pointer
*	@param[out] ret_thread_lock	The address of the thread mutex to be initialized (provides run status feedback). Optional (can be a NULL pointer)
*	@param[in] timeout		Maximum time interval (in ms) to wait for the thread to be running. Only used when a thread mutex is provided for feedback.
*					Must be equal or greater than 0
*
*	@pre				assert( thread_call );
*	@pre				assert( ret_thread_id );
*	@pre				assert( ret_quit_flag );
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*
*	@note				The thread should follow the follow structure
*					@code{.c}
*						void thread_call( void )
*						{
*							pthread_mutex_lock( &thread_mutex );

*							while ( quit_flag == THREAD_RUN ){
*								//do stuff
*							}

*							pthread_mutex_unlock( &thread_mutex );
*							pthread_exit(NULL);
*						}
*					@endcode
*/
int tc_thread_create( void *thread_call, pthread_t *ret_thread_id, char *ret_quit_flag, pthread_mutex_t *ret_thread_lock, unsigned int timeout );

/**	
*	@brief Destroys a thread
*
*	Flags thread to stop and waits for it to end. Destroys feedback mutex if provided
*
*	@param[in] thread_id		The ID of the thread to stop. Must not be a NULL pointer
*	@param[in] quit_flag		The address of the thread quit flag. Must not be a NULL pointer
*	@param[in] thread_lock		The address of the thread mutex to be destroyed (provides run status feedback). Optional (can be a NULL pointer)
*	@param[in] timeout		Maximum time interval (in ms) to wait for the thread to stop before force cancel. Only used when a thread mutex is provided for feedback.
*					Must be equal or greater than 0
*
*	@pre				assert( thread_id );
*	@pre				assert( quit_flag );
*
*	@return 			Upon successful return : ERR_OK (0)
*	@return 			Upon output error : An error code (<0)
*
*	@note				The thread should follow the follow structure
*					@code{.c}
*						void thread_call( void )
*						{
*							pthread_mutex_lock( &thread_mutex );

*							while ( quit_flag == THREAD_RUN ){
*								//do stuff
*							}

*							pthread_mutex_unlock( &thread_mutex );
*							pthread_exit(NULL);
*						}
*					@endcode
*/
int tc_thread_destroy( pthread_t *thread_id, char *quit_flag, pthread_mutex_t *thread_lock, unsigned int timeout );

#endif
