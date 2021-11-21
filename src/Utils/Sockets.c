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

/**	@file Sockets.c
*	@brief Source code of the functions for the sockets layer
*
*	This file contains the implementation of the functions for the sockets layer used for control and topic comunications
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h> 
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <sys/un.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "Sockets.h"
#include "TC_Error_Types.h"
#include "TC_Config.h"
#include "TC_Data_Types.h"

/**	@def DEBUG_MSG_SOCKET
*	@brief If "ENABLE_DEBUG_SOCKET" is defined debug messages related to this module are printed
*/
#if ENABLE_DEBUG_SOCKET
#define DEBUG_MSG_SOCKET(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG_SOCKET(...)
#endif

int sock_open( SOCK_ENTITY *ret_sock, char type )
{
	DEBUG_MSG_SOCKET("sock_open() ...\n");

	int reuse = 1;

	//Check for valid parameters
	if ( !ret_sock ){
		fprintf(stderr,"sock_open() : INVALID SOCKET ENTITY\n");
		return ERR_SOCK_ENTITY;
	}

	//Check for valid socket type
	if ( type != LOCAL && type != REMOTE_UDP && type != REMOTE_TCP && type != REMOTE_UDP_GROUP ){
		fprintf(stderr,"sock_open() : INVALID SOCKET TYPE\n");
		return ERR_SOCK_TYPE;	
	}

	//Clean socket entity struct
	memset(ret_sock,0,sizeof(SOCK_ENTITY));
		
	//Create socket
	if ( type == LOCAL ){
		ret_sock->fd = socket(AF_UNIX, SOCK_DGRAM, 0);
		ret_sock->type = LOCAL;
	}

	if ( type == REMOTE_UDP ){
		ret_sock->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		ret_sock->type = REMOTE_UDP;
	}

	if ( type == REMOTE_TCP ){
		ret_sock->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		ret_sock->type = REMOTE_TCP;
	}

	if ( type == REMOTE_UDP_GROUP ){
		ret_sock->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		ret_sock->type = REMOTE_UDP_GROUP;
	}

	//Check if socket was successfully created
	if ( ret_sock->fd < 0 ){
		perror("sock_open () : ERROR CREATING SOCKET --");
		return ERR_SOCK_CREATE;
	}

	//Set option for reuse
   	if (setsockopt(ret_sock->fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
      		perror("sock_open () : ERROR SETTING SOCKET ADDR REUSE --");
      		close(ret_sock->fd);
      		return ERR_SOCK_OPTION;
	}
	 
	DEBUG_MSG_SOCKET("sock_open() Returning socket entity with fd = %d\n",ret_sock->fd);

	return ERR_OK;
}

int sock_bind( SOCK_ENTITY *sock, NET_ADDR *host ) 
{
	DEBUG_MSG_SOCKET("sock_bind() ...\n");

	//Check if socket entity is valid
	if ( !sock ){
		fprintf(stderr,"sock_bind() : INVALID SOCKET ENTITY\n");
		return ERR_SOCK_ENTITY;
	}

	//Check for valid fd
	if( sock->fd <= 0 ){
		fprintf(stderr,"sock_bind() : INVALID SOCKET FD\n");
		return ERR_SOCK_INVALID_FD;
	}

	//Check if local adress is valid
	if( !host || !strcmp(host->name_ip,"") ){
		fprintf(stderr,"sock_bind() : INVALID GROUP PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Set host address
	if ( sock->type == LOCAL ){
		struct sockaddr_un host_addr;
		memset((char *) &host_addr, 0, sizeof(struct sockaddr_un));

		host_addr.sun_family = AF_UNIX;
		strcpy(host_addr.sun_path, host->name_ip);

		//Unlink name -- Failsafe in case name it is used (I.E. last client crashed without closing the socket)
		unlink(host->name_ip);

		//Bind socket
	  	if ( bind(sock->fd, (struct sockaddr*)&host_addr, sizeof(struct sockaddr_un)) < 0){
	   		perror("sock_bind() : ERROR BINDING SOCKET --");
	   		return ERR_SOCK_BIND_HOST;
		}

	}

	if ( sock->type == REMOTE_UDP || sock->type == REMOTE_TCP || sock->type == REMOTE_UDP_GROUP ){
		struct sockaddr_in host_addr;
		memset((char *) &host_addr, 0, sizeof(struct sockaddr_in));

  		host_addr.sin_family = AF_INET;
  		host_addr.sin_port = htons(host->port);
		host_addr.sin_addr.s_addr  = inet_addr(host->name_ip);//Mandatory for non group communications. Weird behaviours while using both types without this

		if ( sock->type == REMOTE_UDP_GROUP )
			host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

		//Bind socket
  		if ( bind(sock->fd, (struct sockaddr*)&host_addr, sizeof(struct sockaddr_in)) < 0){
   			perror("sock_bind() : ERROR BINDING SOCKET --");
   			return ERR_SOCK_BIND_HOST;
		}
	}

	//Set socket entity
	strcpy( sock->host.name_ip, host->name_ip );
	sock->host.port = host->port;

	if ( sock->type == LOCAL )
		sock->host.port = 0;

	DEBUG_MSG_SOCKET("sock_bind() Socket bound to %s:%d\n",host->name_ip, host->port);

	return ERR_OK;
}

int sock_connect_peer( SOCK_ENTITY *sock, NET_ADDR *peer )
{
	DEBUG_MSG_SOCKET("sock_connect_peer() ...\n");

	//Check if socket entity is valid
	if ( !sock ){
		fprintf(stderr,"sock_connect_peer() : INVALID SOCKET ENTITY\n");
		return ERR_SOCK_ENTITY;
	}

	//Check for valid fd
	if ( sock->fd <= 0 ){
		fprintf(stderr,"sock_connect_peer() : INVALID SOCKET FD\n");
		return ERR_SOCK_INVALID_FD;
	}

	//Check if sock type is valid
	if ( sock->type != LOCAL && sock->type != REMOTE_UDP && sock->type != REMOTE_TCP ){
		fprintf(stderr,"sock_connect_peer() : INVALID SOCKET TYPE ( LOCAL OR REMOTE_UDP/TCP ONLY )\n");
		return ERR_SOCK_TYPE;
	}

	//Check if peer adress is valid
	if ( !peer || !strcmp(peer->name_ip,"") ){
		fprintf(stderr,"sock_connect_peer() : INVALID GROUP PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Set peer address
	if ( sock->type == LOCAL ){
		struct sockaddr_un peer_addr;
		memset((char *) &peer_addr, 0, sizeof(struct sockaddr_un));
	
		peer_addr.sun_family = AF_UNIX;
		strcpy(peer_addr.sun_path, peer->name_ip);

		//Connect to peer
		if ( connect(sock->fd, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr_un)) < 0){
			fprintf(stderr,"sock_connect_peer() : ERROR CONNECTING TO PEER %s:%d",peer->name_ip,peer->port);
			perror("--");
			return ERR_SOCK_CONNECT;
		}

		//Set socket entity
		strcpy( sock->peer.name_ip, peer->name_ip );
		sock->peer.port = 0;
	}

	if ( sock->type == REMOTE_UDP || sock->type == REMOTE_TCP ){
		struct sockaddr_in peer_addr;
		memset((char *) &peer_addr, 0, sizeof(struct sockaddr_in));

		peer_addr.sin_family = AF_INET;
		peer_addr.sin_addr.s_addr = inet_addr(peer->name_ip);
		peer_addr.sin_port = htons(peer->port);

		//Connect to peer
		if ( connect(sock->fd, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr_in)) < 0){
			fprintf(stderr,"sock_connect_peer() : ERROR CONNECTING TO PEER %s:%d",peer->name_ip,peer->port);
			perror("--");
			return ERR_SOCK_CONNECT;
		}

		//Set socket entity
		strcpy( sock->peer.name_ip, peer->name_ip );
		sock->peer.port = peer->port;
	}

	DEBUG_MSG_SOCKET("sock_connect_peer() Connected to peer IP %s on port %d\n",peer->name_ip,peer->port);

	return ERR_OK;
}

int sock_connect_group_rx( SOCK_ENTITY *sock, NET_ADDR *peer )
{
	DEBUG_MSG_SOCKET("sock_connect_group_rx() ...\n");	

	//Check if socket entity is valid
	if ( !sock ){
		fprintf(stderr,"sock_connect_group_rx() : INVALID SOCKET ENTITY\n");
		return ERR_SOCK_ENTITY;
	}

	//Check for valid fd
	if ( sock->fd <= 0 ){
		fprintf(stderr,"sock_connect_group_rx() : INVALID SOCKET FD\n");
		return ERR_SOCK_INVALID_FD;
	}

	//Check for valid socket type
	if ( sock->type != REMOTE_UDP_GROUP ){
		fprintf(stderr,"sock_connect_group_rx() : INVALID SOCKET TYPE -- CAN ONLY BE USED ON REMOTE_GROUP SOCKETS\n");
		return ERR_SOCK_TYPE;
	}

	//Check if socket is bound to host
	if ( !strcmp(sock->host.name_ip,"") || !sock->host.port ){
		fprintf(stderr,"sock_connect_group_rx() : SOCKET NOT BOUND TO HOST\n");
		return ERR_SOCK_BIND_HOST;
	}

	//Check if peer adress is valid
	if ( !peer || !strcmp(peer->name_ip,"") ){
		fprintf(stderr,"sock_connect_group_rx() : INVALID GROUP PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Create MC join request structure
	struct ip_mreq mc_req;
	memset(&mc_req,0,sizeof(struct ip_mreq));

	mc_req.imr_multiaddr.s_addr = inet_addr(peer->name_ip);
	mc_req.imr_interface.s_addr = inet_addr(sock->host.name_ip);

	//Do membership call (to be able to receive. This is a per-host operation and not a process based one)
	if ( setsockopt(sock->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mc_req, sizeof(mc_req)) < 0 ){ 
  		perror("sock_connect_group_rx() : ERROR JOINING MULTICAST GROUP --");
		return ERR_SOCK_OPTION;
	}

	//Set sock entity
	strcpy(sock->peer.name_ip, peer->name_ip);
	sock->peer.port = peer->port;

	DEBUG_MSG_SOCKET("sock_connect_group_rx() Joined %s:%d group as consumer\n",sock->peer.name_ip,sock->peer.port);

	return ERR_OK;
}

int sock_connect_group_tx( SOCK_ENTITY *sock, NET_ADDR *peer )
{
	DEBUG_MSG_SOCKET("sock_connect_group_tx() ...\n");	

	unsigned char mc_ttl = MC_TTL;
	unsigned char loop = MC_LOOP;

	//Check if socket entity is valid
	if ( !sock ){
		fprintf(stderr,"sock_connect_group_tx() : INVALID SOCKET ENTITY\n");
		return ERR_SOCK_ENTITY;
	}

	//Check for valid fd
	if ( sock->fd <= 0 ){
		fprintf(stderr,"sock_connect_group_tx() : INVALID SOCKET FD\n");
		return ERR_SOCK_INVALID_FD;
	}

	//Check for valid socket type
	if ( sock->type != REMOTE_UDP_GROUP ){
		fprintf(stderr,"sock_connect_group_tx() : INVALID SOCKET TYPE -- CAN ONLY BE USED ON REMOTE GROUP SOCKETS\n");
		sock_print_entity( sock );
		return ERR_SOCK_TYPE;
	}

	//Check if socket is bound to host
	if ( !strcmp(sock->host.name_ip,"") || !sock->host.port ){
		fprintf(stderr,"sock_connect_group_tx() : SOCKET NOT BOUND TO HOST\n");
		return ERR_SOCK_BIND_HOST;
	}

	//Check if peer adress is valid
	if ( !peer || !strcmp(peer->name_ip,"") ){
		fprintf(stderr,"sock_connect_group_tx() : INVALID GROUP PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Set the TTL
	if ( setsockopt(sock->fd, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&mc_ttl, sizeof(mc_ttl)) < 0 ){
   		perror("sock_connect_group_tx() : ERROR SETTING MULTICAST TTL --");
   		return ERR_SOCK_OPTION;
  	} 

	//Enable/Disable loopback
	if ( setsockopt(sock->fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0 ){
		perror("sock_connect_group_tx() : ERROR ENABLING/DISABLING LOOPBACK --");
		return ERR_SOCK_OPTION;
	}

	//Set nic interface to send data
	struct in_addr tx_interface;
	memset(&tx_interface,0,sizeof(struct in_addr));

	tx_interface.s_addr = inet_addr(sock->host.name_ip);
	if (setsockopt(sock->fd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&tx_interface, sizeof(tx_interface)) < 0) {
		perror("sock_connect_group_tx() : ERROR SETTING TX INTERFACE --");
		return ERR_SOCK_OPTION;
	}

	//Set sock entity
	strcpy(sock->peer.name_ip, peer->name_ip);
	sock->peer.port = peer->port;
	
	DEBUG_MSG_SOCKET("sock_connect_group_tx() Joined %s:%d group\n",peer->name_ip,peer->port);

	return ERR_OK;
}

int sock_send( SOCK_ENTITY *sock, NET_ADDR *dest, char *data, unsigned int data_size )
{
	DEBUG_MSG_SOCKET("sock_send() ...\n");	

	int ret = -1;

	//Check if socket entity is valid
	if ( !sock ){
		fprintf(stderr,"sock_send() : INVALID SOCKET ENTITY\n");
		return ERR_SOCK_ENTITY;
	}

	//Check for valid fd
	if( sock->fd <= 0 ){
		fprintf(stderr,"sock_send() : INVALID SOCKET FD\n");
		return ERR_SOCK_INVALID_FD;
	}

	//Check if destination adress is valid ( cant be empty if socket isnt connected to a peer )
	if( !strcmp(sock->peer.name_ip,"") && (!dest || !strcmp(dest->name_ip,"")) ){
		fprintf(stderr,"sock_send() : INVALID DESTINATION ADDRESS\n");
		return ERR_INVALID_PARAM;
	}

	//Check for valid data
	if ( !data || !data_size ){
		fprintf(stderr,"sock_send() : INVALID DATA\n");
		return ERR_DATA_INVALID;
	}

	//Set destination address
	if ( sock->type == LOCAL ){
		struct sockaddr_un dest_addr;
		memset((char *) &dest_addr, 0, sizeof(struct sockaddr_un));
	
		dest_addr.sun_family = AF_UNIX;
		strcpy(dest_addr.sun_path, sock->peer.name_ip);
		if ( dest ) 
			strcpy(dest_addr.sun_path, dest->name_ip);

		//Send to destination
		if ( (ret = sendto(sock->fd, data, data_size, 0, (struct sockaddr*)&dest_addr, sizeof(struct sockaddr_un))) < 0){
	   		//perror("sock_send() : ERROR SENDING LOCAL DATA --");
			return ERR_DATA_SEND;
	  	}
	}

	if ( sock->type == REMOTE_UDP || sock->type == REMOTE_TCP || sock->type == REMOTE_UDP_GROUP ){
		struct sockaddr_in dest_addr;
		memset((char *) &dest_addr, 0, sizeof(struct sockaddr_in));

  		dest_addr.sin_family = AF_INET;
		dest_addr.sin_addr.s_addr = inet_addr(sock->peer.name_ip);
		dest_addr.sin_port = htons(sock->peer.port);
  		if ( dest ){
			dest_addr.sin_addr.s_addr = inet_addr(dest->name_ip);
  			dest_addr.sin_port = htons(dest->port);
		}

		//Send to destination
		//printf("going to send to %s:%u\n",sock->peer.name_ip,sock->peer.port);
		if ( (ret = sendto(sock->fd, data, data_size, 0, (struct sockaddr*)&dest_addr, sizeof(struct sockaddr_in))) < 0){
	   		perror("sock_send() : ERROR SENDING REMOTE DATA --");
			return ERR_DATA_SEND;
	  	}
	}

	DEBUG_MSG_SOCKET("sock_send() Sent %d bytes to %s:%d\n",ret,sock->peer.name_ip,sock->peer.port);

	return ret;	
}

int sock_receive( SOCK_ENTITY *sock, SOCK_ENTITY *unblock_sock, unsigned int timeout, char *ret_data, unsigned int buffer_size, NET_ADDR *ret_sender )
{
	DEBUG_MSG_SOCKET("sock_receive() ...\n");	

	int ret = -1, highest_fd;
	struct timeval to, *to_p = NULL;
	fd_set fds;
	int b_size = DEFAULT_MAX_SIZE;

	//Check if socket entity is valid
	if ( !sock ){
		fprintf(stderr,"sock_receive() : INVALID SOCKET ENTITY\n");
		return ERR_SOCK_ENTITY;
	}

	//Check for valid fd
	if( sock->fd <= 0 ){
		fprintf(stderr,"sock_receive() : INVALID SOCKET FD\n");
		return ERR_SOCK_INVALID_FD;
	}

	//Check for valid return data pointer
	if ( !ret_data ){
		fprintf(stderr,"sock_receive() : INVALID RET DATA\n");
		return ERR_DATA_INVALID;
	}

	//Prepare timed-out receive
	FD_ZERO(&fds);
	FD_SET(sock->fd, &fds);
	if ( unblock_sock ) 
		FD_SET(unblock_sock->fd, &fds);

	//Get highest fd
	highest_fd = sock->fd;
	if ( unblock_sock && unblock_sock->fd > sock->fd )
		highest_fd = unblock_sock->fd;

	to.tv_sec = 0;
	to.tv_usec = timeout*1000;//API timeout in ms
	
	if ( timeout > 0 )
		//Don't block indefinitely
		to_p = &to;

	if ( (ret = select(highest_fd+1, &fds, 0, 0, to_p)) <= 0){
		//fprintf(stderr,"sock_receive() : TIMED OUT\n");
		return ERR_DATA_TIMEOUT;
	}

	if ( unblock_sock && FD_ISSET(unblock_sock->fd, &fds) ){
		//Received unlock data (empty buffer)		
		recvfrom( unblock_sock->fd, ret_data, DEFAULT_MAX_SIZE, 0, NULL, NULL);
		return ERR_DATA_UNBLOCK;
	}

	//Set reception buffer size
	if ( buffer_size > 0 )
		b_size = buffer_size;

	if ( sock->type == LOCAL ){
		struct sockaddr_un sender_addr;
		unsigned int size_sender = sizeof(struct sockaddr_un);

		//Received data
		if ( (ret = recvfrom( sock->fd, ret_data, b_size, 0,(struct sockaddr*)&sender_addr, &size_sender)) < 0 ){
	      		perror("sock_receive() : FAILED LOCAL RECEIVE --");
	      		return ERR_DATA_RECEIVE;
	    	}

		//Return sender addr
		if( ret_sender ){
			strcpy(ret_sender->name_ip,sender_addr.sun_path);
			ret_sender->port = 0;
		}

		DEBUG_MSG_SOCKET("sock_receive() Received %d bytes of data from local socket\n",ret);
		return ret;	
	}

	if ( sock->type == REMOTE_UDP || sock->type == REMOTE_TCP || sock->type == REMOTE_UDP_GROUP ){
		struct sockaddr_in sender_addr;
		unsigned int size_sender = sizeof(struct sockaddr_in);

		//Received data
		if ( (ret = recvfrom( sock->fd, ret_data, b_size, 0,(struct sockaddr*)&sender_addr, &size_sender)) < 0 ){
	      		perror("sock_receive() : FAILED REMOTE RECEIVE --");
	      		return ERR_DATA_RECEIVE;
	    	}

		//Return sender addr
		if( ret_sender ){
			strcpy(ret_sender->name_ip,inet_ntoa(sender_addr.sin_addr));
			ret_sender->port = ntohs(sender_addr.sin_port);
		}

		DEBUG_MSG_SOCKET("sock_receive() Received %d bytes of data from remote socket\n",ret);
		return ret;
	}

	fprintf(stderr,"sock_receive() : ERROR RECEIVING DATA -- INVALID SOCKET TYPE\n");
	return ERR_SOCK_TYPE;	
}

int sock_disconnect( SOCK_ENTITY *sock )
{
	DEBUG_MSG_SOCKET("sock_disconnect() ...\n");

	NET_ADDR host;
	char type;

	//Check if socket entity is valid
	if ( !sock ){
		fprintf(stderr,"sock_disconnect() : INVALID SOCKET ENTITY\n");
		return ERR_SOCK_ENTITY;
	}

	//Check for valid fd
	if( sock->fd <= 0 ){
		fprintf(stderr,"sock_disconnect() : INVALID SOCKET FD\n");
		return ERR_SOCK_INVALID_FD;
	}

	//Shutdown communications
	shutdown(sock->fd, SHUT_RDWR);

	//Save sock type and host addr
	type = sock->type;
	host = sock->host;

	//Close this socket and get new one (so we can use bind again)
	//Theres no "unbind" method for sockets that i know of so we do this way
	if ( sock_close ( sock ) < 0 ){
		fprintf(stderr,"sock_disconnect() : ERROR CLOSING SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	if ( sock_open( sock, type ) < 0 ){
		fprintf(stderr,"sock_disconnect() : ERROR CREATING SOCKET\n");
		return ERR_SOCK_CREATE;
	}	

	if ( strcmp(host.name_ip,"") && (sock_bind( sock, &host ) < 0) ){
		fprintf(stderr,"sock_disconnect() : ERROR BINDING SOCKET TO HOST\n");
		return ERR_SOCK_BIND_HOST;
	}		 

	DEBUG_MSG_SOCKET("sock_disconnect() Socket disconneted\n");

	return ERR_OK;
}

int sock_close ( SOCK_ENTITY *sock )
{
	DEBUG_MSG_SOCKET("sock_close() ...\n");

	//Check if socket entity is valid
	if ( !sock ){
		fprintf(stderr,"sock_close() : INVALID SOCKET ENTITY\n");
		return ERR_SOCK_ENTITY;
	}

	//Check for valid fd
	if( sock->fd <= 0 ){
		fprintf(stderr,"sock_close() : INVALID SOCKET FD\n");
		return ERR_SOCK_INVALID_FD;
	}

	//Shutdown communications
	shutdown(sock->fd, SHUT_RDWR);

	//Close socket
	if ( close(sock->fd) < 0 ){
		fprintf(stderr,"sock_close(): ERROR CLOSING SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	sock->fd = 0;

	//Unlink names
	if ( sock->type == LOCAL )
		unlink(sock->host.name_ip);

	DEBUG_MSG_SOCKET("sock_close() Returning 0\n");

	return ERR_OK;
}

int sock_print_entity( SOCK_ENTITY *sock )
{
	if ( !sock ){
		fprintf(stderr,"sock_print_entity() : INVALID ENTITY\n");
		return -1;
	}

	printf("\n--SOCKET ENTITY--\n");
	printf("sock->fd = %d\n",sock->fd);
	if ( sock->type == LOCAL ) printf("sock->type = %s\n","local");
	if ( sock->type == REMOTE_UDP ) printf("sock->type = %s\n","remote");
	if ( sock->type == REMOTE_UDP_GROUP ) printf("sock->type = %s\n","remote group");

	printf("\nsock->host.name_ip = %s\n",sock->host.name_ip);
	printf("sock->host.port = %u\n",sock->host.port);

	printf("\nsock->peer.name_ip = %s\n",sock->peer.name_ip);
	printf("sock->peer.port = %u\n",sock->peer.port);

	return 0;
}
