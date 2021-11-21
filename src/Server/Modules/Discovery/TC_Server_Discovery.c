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

/**	@file TC_Server_Discovery.c
*	@brief Source code of the functions for the server discovery module
*
*	This file contains the implementation of the functions for the server discovery module. 
*	This module sends periodically discovery messages to all the possible listening nodes. This messages
*	contain the servers configured address. 
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

#include "TC_Server_Discovery.h"

#include "Sockets.h"
#include "TC_Data_Types.h"
#include "TC_Utils.h"
#include "TC_Config.h"
#include "TC_Error_Types.h"

/**	@def DEBUG_MSG_SERVER_DISCOVERY
*	@brief If "ENABLE_DEBUG_SERVER_DISCOVERY" is defined debug messages related to this module are printed
*/
#if ENABLE_DEBUG_SERVER_DISCOVERY
#define DEBUG_MSG_SERVER_DISCOVERY(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG_SERVER_DISCOVERY(...)
#endif

static char init = 0;
static char quit = 0;

static pthread_t discovery_thread_id;
static pthread_mutex_t discovery_lock;
static SOCK_ENTITY remote_sock;

static pthread_t discovery_local_thread_id;
static pthread_mutex_t discovery_local_lock;
static SOCK_ENTITY local_sock;

static NET_ADDR server_addr;

//Periodically sends discovery message
static void discovery_generator( void );
static void discovery_local_generator( void );

int tc_server_discovery_init( NET_ADDR *server_remote )
{
	DEBUG_MSG_SERVER_DISCOVERY("tc_server_discovery_init() ...\n");

	if ( init ){
		fprintf(stderr,"tc_server_discovery_init() : MODULE ALREADY INITIALIZED\n");
		return ERR_S_ALREADY_INIT;
	}

	assert( server_remote );
	assert( server_remote->name_ip );
	assert( server_remote->port );

	//Create socket for local discovery
	if ( sock_open( &local_sock, LOCAL ) < 0){
		fprintf(stderr,"tc_server_discovery_init() : ERROR CREATING LOCAL SERVER SOCKET\n");
		return ERR_SOCK_CREATE;
	}

	//Set and bind to local host address
	NET_ADDR host = {SERVER_DISCOVERY_LOCAL_FILE,0};
	if ( sock_bind( &local_sock, &host ) ){
		fprintf(stderr,"tc_server_discovery_init() : ERROR BINDING SOCKET TO LOCAL HOST ADDRESS\n");
		sock_close(&local_sock);
		return ERR_SOCK_BIND_HOST;
	}

	//Create remote server socket
	if ( sock_open( &remote_sock, REMOTE_UDP_GROUP ) < 0){
		fprintf(stderr,"tc_server_discovery_init() : ERROR CREATING SERVER SOCKET\n");
		return ERR_SOCK_CREATE;
	}

	//Set and bind to remote host address
	strcpy(host.name_ip,server_remote->name_ip);
	host.port = DISCOVERY_GROUP_PORT;

	if ( sock_bind( &remote_sock, &host ) ){
		fprintf(stderr,"tc_server_discovery_init() : ERROR BINDING SOCKET TO REMOTE HOST ADDRESS\n");
		sock_close( &local_sock );
		sock_close( &remote_sock );
		return ERR_SOCK_BIND_HOST;
	}

	//Set remote address
	NET_ADDR peer;
	strcpy(peer.name_ip, DISCOVERY_GROUP_IP);
	peer.port = DISCOVERY_GROUP_PORT;	
	
	//Join discovery group
	if ( sock_connect_group_tx( &remote_sock, &peer ) ){
		fprintf(stderr,"tc_server_discovery_init() : ERROR REGISTERING TO DISCOVERY GROUP\n");
		sock_close ( &local_sock );
		sock_close ( &remote_sock );
		return ERR_SOCK_BIND_PEER;
	}

	//Set server address
	memset(&server_addr,0,sizeof(NET_ADDR));
	strcpy(server_addr.name_ip,server_remote->name_ip);
	server_addr.port = server_remote->port;

	//Create local discovery thread
	if ( tc_thread_create( discovery_local_generator, &discovery_local_thread_id, &quit, &discovery_local_lock, 100 ) ){
		fprintf(stderr,"tc_server_discovery_init() : ERROR CREATING LOCAL DISCOVERY THREAD\n");
		sock_close( &local_sock );
		sock_close( &remote_sock );
		return ERR_THREAD_CREATE;
	}

	//Create remote discovery thread
	if ( tc_thread_create( discovery_generator, &discovery_thread_id, &quit, &discovery_lock, 100 ) ){
		fprintf(stderr,"tc_server_discovery_init() : ERROR CREATING REMOTE DISCOVERY THREAD\n");
		sock_close( &local_sock );
		sock_close( &remote_sock );
		return ERR_THREAD_CREATE;
	}

	init = 1;
	
	DEBUG_MSG_SERVER_DISCOVERY("tc_server_discovery_init() Discovery module started\n");

	return ERR_OK;
}

int tc_server_discovery_close( void )
{
	DEBUG_MSG_SERVER_DISCOVERY("tc_server_discovery_close() ...\n");

	if ( !init ){
		fprintf(stderr,"tc_server_discovery_close() : MODULE ISNT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	//Stop local discovery thread
	if ( tc_thread_destroy( &discovery_local_thread_id, &quit, &discovery_local_lock, 100 ) ){
		fprintf(stderr,"tc_server_discovery_close() : ERROR DESTROYING LOCAL DISCOVERY THREAD\n");
		return ERR_THREAD_DESTROY;
	}

	//Stop remote discovery thread
	if ( tc_thread_destroy( &discovery_thread_id, &quit, &discovery_lock, 100 ) ){
		fprintf(stderr,"tc_server_discovery_close() : ERROR DESTROYING REMOTE DISCOVERY THREAD\n");
		return ERR_THREAD_DESTROY;
	}

	//Close local discovery socket
	if ( sock_close ( &local_sock ) ){
		fprintf(stderr,"tc_server_discovery_close() : ERROR CLOSING LOCAL DISCOVERY SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	if ( sock_close( &remote_sock ) ){
		fprintf(stderr,"tc_server_discovery_close() : ERROR CLOSING REMOTE DISCOVERY SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	init = 0;

	DEBUG_MSG_SERVER_DISCOVERY("tc_server_discovery_close() Discovery module closed\n");

	return ERR_OK;
}

static void discovery_local_generator( void )
{
	DEBUG_MSG_SERVER_DISCOVERY("discovery_local_generator() ...\n");

	NET_MSG discovery_msg;
	NET_ADDR peer = {CLIENT_DISCOVERY_LOCAL_FILE,0};

	pthread_mutex_lock( &discovery_local_lock );

	discovery_msg.type = DIS_MSG;
	strcpy( discovery_msg.topic_addr.name_ip, SERVER_AC_LOCAL_FILE );
	discovery_msg.topic_addr.port = 0;

	while ( quit == THREAD_RUN ){

		tc_network_send_msg( &local_sock, &discovery_msg, &peer );

		DEBUG_MSG_SERVER_DISCOVERY("discovery_local_generator() : Sent server addr %s:%u\n",discovery_msg.topic_addr.name_ip,discovery_msg.topic_addr.port);
		usleep(DISCOVERY_GEN_PERIOD);
	}

	pthread_mutex_unlock( &discovery_local_lock );

	DEBUG_MSG_SERVER_DISCOVERY("discovery_local_generator() Discovery thread ending\n");

	pthread_exit(NULL);
}

static void discovery_generator( void )
{
	DEBUG_MSG_SERVER_DISCOVERY("discovery_generator() ...\n");

	NET_MSG discovery_msg;

	pthread_mutex_lock( &discovery_lock );

	discovery_msg.type = DIS_MSG;
	strcpy( discovery_msg.topic_addr.name_ip, server_addr.name_ip );
	discovery_msg.topic_addr.port = server_addr.port;

	NET_ADDR peer = {DISCOVERY_GROUP_IP,DISCOVERY_GROUP_PORT};

	while ( quit == THREAD_RUN ){

		tc_network_send_msg( &remote_sock, &discovery_msg, &peer );

		DEBUG_MSG_SERVER_DISCOVERY("discovery_generator() : Sent server addr %s:%u\n",discovery_msg.topic_addr.name_ip,discovery_msg.topic_addr.port);
		usleep(DISCOVERY_GEN_PERIOD);
	}

	pthread_mutex_unlock( &discovery_lock );

	DEBUG_MSG_SERVER_DISCOVERY("discovery_generator() Discovery thread ending\n");

	pthread_exit(NULL);
}
