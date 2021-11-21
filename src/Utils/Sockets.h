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

/**	@file Sockets.h
*	@brief Function prototypes for the sockets layer
*
*	This file contains the function prototypes for the sockets layer used for control and topic comunications
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef SOCKETS_H
#define SOCKETS_H

#include "TC_Data_Types.h"

/** 	@def DEFAULT_MAX_SIZE
*	@brief Default size for data reception buffer in sockets
*/
#define DEFAULT_MAX_SIZE 65535

//#define MIN_PORT 1024 //In unix only users with root can use sockets binded to port number below 1024
//#define MAX_PORT 65535

/** 	@def MC_TTL
*	@brief Multicast time to live
*/
#define MC_TTL	1

/** 	@def MC_LOOP
*	@brief Multicast loop (to loopback the sent data). If 0 loopback is disabled. If 1 loopback is enabled
*/
#define MC_LOOP	0


/**	
*	@brief Creates a new socket
*
*	Creates a new socket with the desired type
*
*	@param[out] ret_sock	The buffer where to store the created socket information. Must not be a NULL pointer
*	@param[in] type 	The type of the socket to be created. Must be one of the enumerated types (LOCAL,REMOTE_UDP,REMOTE_TCP,REMOTE_UDP_GROUP)
*
*	@pre			None
*
*	@return 		Upon successful return : ERR_OK
*	@return 		Upon output error : An error code (<0)
*/
int sock_open( SOCK_ENTITY *ret_sock, char type );

/**	
*	@brief Binds a socket to the host
*
*	Binds a socket to the host address
*
*	@param[in] sock		The socket to be bound. Must not be a NULL pointer
*	@param[in] host 	The host adress to bind to. Must not  be a NULL pointer
*
*	@pre			None
*
*	@return 		Upon successful return : ERR_OK
*	@return 		Upon output error : An error code (<0)
*/
int sock_bind( SOCK_ENTITY *sock, NET_ADDR *host ) ;

/**	
*	@brief Connects a socket a peer
*
*	Connects a socket to the peer address
*
*	@param[in] sock		The socket to be connected. Must not be a NULL pointer
*	@param[in] peer 	The peer adress to connect to. Must not  be a NULL pointer
*
*	@pre			None
*
*	@return 		Upon successful return : ERR_OK
*	@return 		Upon output error : An error code (<0)
*
*	@note			It is advised to use this on previously bound sockets (but not necessary)
*/
int sock_connect_peer( SOCK_ENTITY *sock, NET_ADDR *peer );

/**	
*	@brief Connects a socket to a group address as consumer
*
*	Connects a socket to a multicast group address as consumer
*
*	@param[in] sock		The socket to be connected. Must not be a NULL pointer
*	@param[in] peer 	The multicast group adress to connect to. Must not  be a NULL pointer
*
*	@pre			None
*
*	@return 		Upon successful return : ERR_OK
*	@return 		Upon output error : An error code (<0)
*
*	@note			The socket had to be previously bound to the host address (matching the same group port). Both this and "sock_connect_group_tx"
*				can be called for the same socket making it both producer and consumer of the group (but without loopback)
*/
int sock_connect_group_rx( SOCK_ENTITY *sock, NET_ADDR *peer );

/**	
*	@brief Connects a socket to a group address as producer
*
*	Connects a socket to a multicast group address as producer
*
*	@param[in] sock		The socket to be connected. Must not be a NULL pointer
*	@param[in] peer 	The multicast group adress to connect to. Must not  be a NULL pointer
*
*	@pre			None
*
*	@return 		Upon successful return : ERR_OK
*	@return 		Upon output error : An error code (<0)
*
*	@note			The socket had to be previously bound to the host address (matching the same group port). Both this and "sock_connect_group_rx"
*				can be called for the same socket making it both producer and consumer of the group (but without loopback)
*/
int sock_connect_group_tx( SOCK_ENTITY *sock, NET_ADDR *peer );

/**	
*	@brief Sends data through a socket
*
*	Sends data through a connected/unconnected socket
*
*	@param[in] sock		The socket to send the data through. Must not be a NULL pointer
*	@param[in] dest 	The multicast group adress to connect to. Can be a NULL pointer if socket is connected to a peer
*	@param[in] data 	The buffer with the data to be sent. Must not  be a NULL pointer
*	@param[in] data_size 	The size of data (in bytes). Must not  be a NULL pointer
*
*	@pre			None
*
*	@return 		Upon successful return : The number of sent bytes
*	@return 		Upon output error : An error code (<0)
*
*	@note			The socket had to be previously \b bound and \b connected to the group address for \b multicast comunications. For unconnected sockets and point to point
*				comunications \a dest has to be filled with the destination address.
*/	
int sock_send( SOCK_ENTITY *sock, NET_ADDR *dest, char *data, unsigned int data_size );

/**	
*	@brief Receives data from a socket
*
*	Receives data through a connected/unconnected socket
*
*	@param[in] sock		The socket to receive the data from. Must not be a NULL pointer
*	@param[in] unblock_sock The socket from where to receive an unblock signal. Optional (can be a NULL pointer)
*	@param[in] timeout 	The maximum time interval (in ms) to wait for data. If 0 blocks indefinitely. Must be equal or greater than 0
*	@param[out] ret_data 	The buffer to store the received data. Must not  be a NULL pointer
*	@param[in] buff_size 	The size of the data buffer. Must be greater than 0 else a default size is considered (DEFAULT_MAX_SIZE)
*	@param[out] ret_sender 	The buffer to store the address of the sender. Optional (can be a NULL pointer)
*
*	@pre			None
*
*	@return 		Upon successful return : The number of received bytes
*	@return 		Upon output error : An error code (<0)
*
*	@note			The socket had to be previously \b bound and \b connected to the group address for \b multicast comunications. For unconnected sockets and point to point
*				comunications \a dest has to be filled with the destination address.
*/	
int sock_receive( SOCK_ENTITY *sock, SOCK_ENTITY *unblock_sock, unsigned int timeout, char *ret_data, unsigned int buff_size, NET_ADDR *ret_sender );

/**	
*	@brief Disconnects a socket from the peer
*
*	Disconnects a socket from the peer
*
*	@param[in] sock		The socket to be disconnected. Must not be a NULL pointer
*
*	@pre			None
*
*	@return 		Upon successful return : ERR_OK
*	@return 		Upon output error : An error code (<0)
*
*	@note			The socket will still be bound to the host after this call
*/	
int sock_disconnect( SOCK_ENTITY *sock );

/**	
*	@brief Closes a socket
*
*	Closes and frees a socket
*
*	@param[in] sock		The socket to be closed. Must not be a NULL pointer
*
*	@pre			None
*
*	@return 		Upon successful return : ERR_OK
*	@return 		Upon output error : An error code (<0)
*
*	@note			The \a sock buffer won't be freed
*/	
int sock_close ( SOCK_ENTITY *sock );

/**	
*	@brief Prints a socket information
*
*	Prints a socket entity information on stdout
*
*	@param[in] sock		The socket which information is to be printed. Must not be a NULL pointer
*
*	@pre			None
*
*	@return 		Upon successful return : ERR_OK
*	@return 		Upon output error : An error code (<0)
*/	
int sock_print_entity( SOCK_ENTITY *sock );

#endif
