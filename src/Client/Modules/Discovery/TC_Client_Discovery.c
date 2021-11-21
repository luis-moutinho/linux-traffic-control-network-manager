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

/**	@file TC_Client_Discovery.c
*	@brief Source code of the functions for the client discovery module
*
*	This file contains the implementation of the functions for the client discovery
*	module. This module implements mechanisms to discover a server and retrieve its address. This server can be local to the node or in the network.
*	Internal module
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#include "TC_Client_Discovery.h"

#include "Sockets.h"
#include "TC_Data_Types.h"
#include "TC_Utils.h"
#include "TC_Config.h"
#include "TC_Error_Types.h"

/** 	@def DEBUG_MSG_CLIENT_DISCOVERY
*	@brief If "ENABLE_DEBUG_CLIENT_DISCOVERY" is defined debug messages related to this module are printed
*/
#if ENABLE_DEBUG_CLIENT_DISCOVERY
#define DEBUG_MSG_CLIENT_DISCOVERY(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG_CLIENT_DISCOVERY(...)
#endif

static SOCK_ENTITY remote_sock, local_sock;
static char init = 0;

int tc_client_discovery_init( char *ifface )
{
	DEBUG_MSG_CLIENT_DISCOVERY("tc_client_discovery_init() ...\n");

	char nic_ip[50];

	assert( ifface );

	if ( init ){
		fprintf(stderr,"tc_client_discovery_init() : MODULE ALREADY INITIALIZED\n");
		return ERR_C_ALREADY_INIT;
	}

	//Get nic ip address
	if ( tc_network_get_nic_ip( ifface, nic_ip ) < 0){
		fprintf(stderr,"tc_client_discovery_init() : ERROR GETTING NIC IP ADDRESS\n");
		return ERR_INVALID_NIC;
	}

	//Create socket for local discovery
	if ( sock_open( &local_sock, LOCAL ) < 0){
		fprintf(stderr,"tc_client_discovery_init() : ERROR CREATING LOCAL SERVER SOCKET\n");
		return ERR_SOCK_CREATE;
	}

	//Set and bind to host address
	NET_ADDR host = {CLIENT_DISCOVERY_LOCAL_FILE,0};
	if ( sock_bind( &local_sock, &host ) ){
		fprintf(stderr,"tc_client_discovery_init() : ERROR BINDING SOCKET TO HOST ADDRESS\n");
		sock_close( &local_sock );
		return ERR_SOCK_BIND_HOST;
	}

	//Create remote server socket
	if ( sock_open( &remote_sock, REMOTE_UDP_GROUP ) < 0){
		fprintf(stderr,"tc_client_discovery_init() : ERROR CREATING SERVER SOCKET\n");
		return ERR_SOCK_CREATE;
	}

	//Set and bind to host address
	strcpy(host.name_ip, nic_ip);
	host.port = DISCOVERY_GROUP_PORT;

	if ( sock_bind( &remote_sock, &host ) ){
		fprintf(stderr,"tc_client_discovery_init() : ERROR BINDING SOCKET TO LOCAL ADDRESS\n");
		sock_close ( &local_sock );
		sock_close ( &remote_sock );
		return ERR_SOCK_BIND_HOST;
	}

	//Set remote address
	NET_ADDR peer;
	strcpy(peer.name_ip, DISCOVERY_GROUP_IP);
	peer.port = DISCOVERY_GROUP_PORT;	
	
	//Join discovery group
	if ( sock_connect_group_rx( &remote_sock, &peer ) ){
		fprintf(stderr,"tc_client_discovery_init() : ERROR REGISTERING TO DISCOVERY GROUP\n");
		sock_close ( &local_sock );
		sock_close ( &remote_sock );
		return ERR_SOCK_BIND_PEER;
	}

	init = 1;

	DEBUG_MSG_CLIENT_DISCOVERY("tc_client_discovery_init() Module initialized\n");

	return ERR_OK;
}

int tc_client_discovery_close( void )
{
	DEBUG_MSG_CLIENT_DISCOVERY("tc_client_discovery_close() ...\n");

	if ( !init ){
		fprintf(stderr,"tc_client_discovery_close() : MODULE ISNT RUNNING\n");
		return ERR_C_NOT_INIT;
	}

	//Close sockets
	sock_close ( &local_sock );
	sock_close ( &remote_sock );

	init = 0;

	DEBUG_MSG_CLIENT_DISCOVERY("tc_client_discovery_close() Module closed\n");

	return ERR_OK;
}

int tc_client_discovery_find_server( unsigned int timeout, NET_ADDR *ret_server )
{
	DEBUG_MSG_CLIENT_DISCOVERY("tc_client_discovery_find_server() ...\n");

	NET_MSG msg;
	int highest_fd;
	struct timeval to, *to_p = NULL;

	assert( ret_server );

	if ( !init ){
		fprintf(stderr,"tc_client_discovery_find_server() : MODULE ISNT RUNNING\n");
		return ERR_C_NOT_INIT;
	}

	//Find server
	//Prepare timed-out receive
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(local_sock.fd, &fds);
	FD_SET(remote_sock.fd, &fds);

	//Get highest fd
	highest_fd = local_sock.fd;
	if ( remote_sock.fd > local_sock.fd )
		highest_fd = remote_sock.fd;

	to.tv_sec = 0;
	to.tv_usec = timeout*1000;//API timeout in ms
	
	if ( timeout > 0 )
		//Don't block indefinitely
		to_p = &to;

	if ( select(highest_fd+1, &fds, 0, 0, to_p) <= 0){
		fprintf(stderr,"tc_client_discovery_find_server() : COULDN'T GET DISCOVERY MESSAGE\n");
		return ERR_DATA_TIMEOUT;
	}

	memset(&msg,0,sizeof(NET_MSG));

	if ( FD_ISSET(local_sock.fd, &fds) ){
		//Server is in the same local node
		tc_network_get_msg( &local_sock, 0, &msg, NULL );
	}else{
		//Server is in a remote node	
		tc_network_get_msg( &remote_sock, 0, &msg, NULL );
	}

	//Check if received message is a discovery one
	if( msg.type != DIS_MSG ){
		fprintf(stderr,"tc_client_discovery_find_server() : INVALID DISCOVERY MESSAGE\n");
		return -4;
	}

	//Store server address
	strcpy(ret_server->name_ip,msg.topic_addr.name_ip);
	ret_server->port = msg.topic_addr.port;

	DEBUG_MSG_CLIENT_DISCOVERY("tc_client_discovery_find_server() Found server\n");

	return ERR_OK;
}

