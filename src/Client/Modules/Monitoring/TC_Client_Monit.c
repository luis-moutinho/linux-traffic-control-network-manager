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

/**	@file TC_Client_Monit.c
*	@brief Source code of the functions for the client monitoring module
*
*	This file contains the implementation of the functions for the client monitoring module. 
*	This module periodically sends heartbeat messages to the server monitoring module to keep the node registered in the network.
*	It also (using the discovery module) periodically checks for the server. If the server is disconnected from the network this module raises
*	a signal to close the client.Internal module
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
#include <signal.h>

#include "TC_Client_Monit.h"
#include "TC_Error_Types.h"
#include "Sockets.h"
#include "TC_Utils.h"
#include "TC_Config.h"
#include "TC_Client_Discovery.h"

/** 	@def DEBUG_MSG_CLIENT_MONIT
*	@brief If "ENABLE_DEBUG_CLIENT_MONIT" is defined debug messages related to this module are printed
*/
#if ENABLE_DEBUG_CLIENT_MONIT
#define DEBUG_MSG_CLIENT_MONIT(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG_CLIENT_MONIT(...)
#endif

static char init = 0;
static char quit = 0;

static pthread_t monit_thread_id;
static pthread_mutex_t monit_lock;

static pthread_t monit_server_thread_id;
static pthread_mutex_t monit_server_lock;

static char nic_ip[50];

static unsigned int monit_node_id = 0;
static SOCK_ENTITY sock; 

static void monitor( void );
static int monitor_tick( unsigned int node_id );
static void monitor_server( void );

int tc_client_monit_init( char *ifface, unsigned int node_id, NET_ADDR *server )
{
	DEBUG_MSG_CLIENT_MONIT("tc_client_monit_init() ...\n");

	if ( init ){
		fprintf(stderr,"tc_client_monit_init() : MODULE ALREADY INITIALIZED\n");
		return ERR_C_ALREADY_INIT;
	}

	assert( server );

	//Get nic ip address
	if ( tc_network_get_nic_ip( ifface, nic_ip ) ){
		fprintf(stderr,"tc_client_monit_init() : ERROR GETTING NIC IP ADDRESS\n");
		return ERR_INVALID_NIC;
	}

	//Create monitoring socket
	if ( !server->port ){
		//Server is in the same local node

		//Create local server socket
		if ( sock_open( &sock, LOCAL ) < 0){
			fprintf(stderr,"tc_client_monit_init() : ERROR CREATING LOCAL SERVER SOCKET\n");
			return ERR_SOCK_CREATE;
		}

		//Set and bind to host address
		NET_ADDR host = {CLIENT_MONITORING_LOCAL_FILE,0};
		if ( sock_bind( &sock, &host ) ){
			fprintf(stderr,"tc_client_monit_init() : ERROR BINDING SOCKET TO HOST ADDRESS\n");
			sock_close(&sock);
			return ERR_SOCK_BIND_HOST;
		}

		//Connect socket to server
		NET_ADDR peer = {SERVER_MONITORING_LOCAL_FILE,0};
		if ( sock_connect_peer( &sock, &peer ) ){
			fprintf(stderr,"tc_client_monit_init() : ERROR CONNECTING LOCAL SOCKET TO SERVER\n");
			sock_close ( &sock );
			return ERR_SOCK_BIND_PEER;
		}

	}else{
		//Server is in a remote node

		//Create remote server socket
		if ( sock_open( &sock, REMOTE_UDP ) < 0){
			fprintf(stderr,"tc_client_monit_init() : ERROR CREATING SERVER SOCKET\n");
			return ERR_SOCK_CREATE;
		}

		//Set and bind to host address
		NET_ADDR host;
		strcpy(host.name_ip,nic_ip);
		host.port = server->port + MONITORING_PORT_OFFSET;

		if ( sock_bind( &sock, &host ) ){
			fprintf(stderr,"tc_client_monit_init() : ERROR BINDING SOCKET TO LOCAL ADDRESS\n");
			sock_close( &sock );
			return ERR_SOCK_BIND_HOST;
		}

		//Set remote address
		NET_ADDR peer;
		strcpy(peer.name_ip, server->name_ip);
		peer.port = server->port + MONITORING_PORT_OFFSET;

		//Connect to server
		if ( sock_connect_peer( &sock, &peer ) ){
			fprintf(stderr,"tc_client_monit_init() : ERROR CONNECTING TO SERVER -- WRONG SERVER IP/PORT?\n");
			sock_close( &sock );
			return ERR_SOCK_BIND_PEER;
		}
	}

	//Launch monitoring thread
	if ( tc_thread_create( monitor, &monit_thread_id, &quit, &monit_lock, 100 ) ){
		fprintf(stderr,"tc_client_monit_init() : ERROR CREATING MONITORING THREAD\n");
		sock_close( &sock );
		return ERR_THREAD_CREATE;
	}

	//Launch server monitoring thread
	if ( tc_thread_create( monitor_server, &monit_server_thread_id, &quit, &monit_server_lock, 100 ) ){
		fprintf(stderr,"tc_client_monit_init() : ERROR CREATING SERVER MONITORING THREAD\n");
		tc_thread_destroy( &monit_thread_id, &quit, &monit_lock, 100);
		sock_close( &sock );
		return ERR_THREAD_CREATE;
	}

	monit_node_id = node_id;
	init = 1;
	
	DEBUG_MSG_CLIENT_MONIT("tc_client_monit_init() Monitoring module initialized\n");

	return ERR_OK;
}

int tc_client_monit_close( void )
{
	DEBUG_MSG_CLIENT_MONIT("tc_client_monit_close() ...\n");

	if ( !init ){
		fprintf(stderr,"tc_client_monit_close() : MODULE ISNT RUNNING\n");
		return ERR_C_NOT_INIT;
	}

	//Stop monitoring thread
	if ( tc_thread_destroy( &monit_thread_id, &quit, &monit_lock, 100) ){
		fprintf(stderr,"tc_client_monit_close() : ERROR DESTROYING MONITORING THREAD\n");
		return ERR_THREAD_DESTROY;
	}

	//Stop server monitoring thread
	if ( tc_thread_destroy( &monit_server_thread_id, &quit, &monit_server_lock, 100) ){
		fprintf(stderr,"tc_client_monit_close() : ERROR DESTROYING SERVER MONITORING THREAD\n");
		return ERR_THREAD_DESTROY;
	}
		
	if ( sock_close( &sock ) ){
		fprintf(stderr,"tc_client_monit_close() : ERROR CLOSING REMOTE SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	init = 0;
	monit_node_id = -1;

	DEBUG_MSG_CLIENT_MONIT("tc_client_monit_close() Monitoring module closed\n");

	return ERR_OK;
}

static void monitor( void )
{
	DEBUG_MSG_CLIENT_MONIT("monitor() ...\n");

	pthread_mutex_lock( &monit_lock );

	while ( quit == THREAD_RUN ){
		DEBUG_MSG_CLIENT_MONIT("monitor() Sending tick of Node Id %u\n",monit_node_id);
		monitor_tick( monit_node_id );
		usleep(HEARTBEAT_GEN_PERIOD);
	}

	pthread_mutex_unlock( &monit_lock );

	DEBUG_MSG_CLIENT_MONIT("monitor() Monitoring thread ending\n");

	pthread_exit(NULL);
}

static void monitor_server( void )
{
	DEBUG_MSG_CLIENT_MONIT("monitor_server() ...\n");

	NET_ADDR server;

	pthread_mutex_lock( &monit_server_lock );

	while ( quit == THREAD_RUN ){

		usleep(DISCOVERY_GEN_PERIOD);

		if ( tc_client_discovery_find_server( DISCOVERY_GEN_PERIOD/1000 /*ms*/, &server ) ){
			fprintf(stderr,"monitor_server() : SERVER UNPLUGED FROM NETWORK -- GOING TO SEND SIGNAL\n");
			raise(SIGUSR1);
		}
	}

	pthread_mutex_unlock( &monit_server_lock );

	DEBUG_MSG_CLIENT_MONIT("monitor_server() Monitoring thread ending\n");

	pthread_exit(NULL);
}

static int monitor_tick( unsigned int node_id )
{
	DEBUG_MSG_CLIENT_MONIT("monitor_tick() ...\n");

	NET_MSG msg;

	if ( !init ){
		fprintf(stderr,"monitor_tick() : MODULE ISNT RUNNING\n");
		return ERR_C_NOT_INIT;
	}

	//Set msg
	memset(&msg,0,sizeof(NET_MSG));

	msg.type = REQ_MSG;
	msg.op = HEART_SIG;
	msg.node_ids[0] = monit_node_id;
	msg.n_nodes = 1;

	DEBUG_MSG_CLIENT_MONIT("monitor_tick() Sending request %c to %s:%u\n",msg.op,sock.peer.name_ip,sock.peer.port);

	tc_network_send_msg( &sock, &msg, NULL );

	DEBUG_MSG_CLIENT_MONIT("monitor_tick() Sent node id %u heartbeat tick to server\n",node_id);

	return ERR_OK;
}
