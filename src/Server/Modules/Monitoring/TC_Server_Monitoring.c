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

/**	@file TC_Server_Monitoring.c
*	@brief Source code of the functions for the server monitoring module
*
*	This file contains the implementation of the functions for the server monitoring module. 
*	This module receives the heartbeat request messages from clients to reset their heartbeat counter. This module also periodically decrements
*	all nodes counter and signals a dead node when its counter reaches 0.
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

#include "TC_Data_Types.h"
#include "TC_Utils.h"
#include "Sockets.h"
#include "TC_Server_DB.h"
#include "TC_Server_Monitoring.h"
#include "TC_Server_Management.h"
#include "TC_Server_Notifications.h"
#include "TC_Config.h"
#include "TC_Error_Types.h"

/**	@def DEBUG_MSG_SERVER_MONIT
*	@brief If "ENABLE_DEBUG_SERVER_MONIT" is defined debug messages related to this module are printed
*/
#if ENABLE_DEBUG_SERVER_MONIT
#define DEBUG_MSG_SERVER_MONIT(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG_SERVER_MONIT(...)
#endif

static char init = 0;
static char quit = 0;

static pthread_t tock_thread_id;
static pthread_mutex_t tock_lock;

static pthread_t tick_thread_id;
static pthread_mutex_t tick_lock;

//Periodically calls tc_server_monit_tock 
static void tc_server_monit_tock_thread( void );

//Receives node heartbeat requests and calls tc_server_monit_tick
static void tc_server_monit_tick_thread( void );

//Decrement all node heartbeat counters. Upon reaching <= 0 node is declared dead and management module is called to remove node
static int tc_server_monit_tock( void );

//Resets node heartbeat counter
static int tc_server_monit_tick( unsigned int node_id );

static SOCK_ENTITY monit_local_sock;
static SOCK_ENTITY monit_remote_sock;

int tc_server_monitoring_init( NET_ADDR *server_remote )
{
	DEBUG_MSG_SERVER_MONIT("tc_server_monitoring_init() ...\n");

	if ( init ){
		fprintf(stderr,"tc_server_monitoring_init() : MODULE ALREADY INITIALIZED\n");
		return ERR_S_ALREADY_INIT;
	}

	assert( server_remote );
	assert( server_remote->name_ip );
	assert( server_remote->port );

	//Create local server socket
	if ( sock_open( &monit_local_sock, LOCAL ) < 0){
		fprintf(stderr,"tc_server_monitoring_init() : ERROR CREATING LOCAL SERVER SOCKET\n");
		return ERR_SOCK_CREATE;
	}

	//Set and bind to local host address
	NET_ADDR host = {SERVER_MONITORING_LOCAL_FILE,0};
	if ( sock_bind( &monit_local_sock, &host ) ){
		fprintf(stderr,"tc_server_monitoring_init() : ERROR BINDING SOCKET TO LOCAL HOST ADDRESS\n");
		sock_close(&monit_local_sock);
		return ERR_SOCK_BIND_HOST;
	}

	//Create remote server socket
	if ( sock_open( &monit_remote_sock, REMOTE_UDP ) < 0){
		fprintf(stderr,"tc_server_monitoring_init() : ERROR CREATING SERVER SOCKET\n");
		sock_close( &monit_local_sock );
		return ERR_SOCK_CREATE;
	}

	//Set and bind to host address
	strcpy(host.name_ip,server_remote->name_ip);
	host.port = server_remote->port + MONITORING_PORT_OFFSET;

	if ( sock_bind( &monit_remote_sock, &host ) ){
		fprintf(stderr,"tc_server_monitoring_init() : ERROR BINDING SOCKET TO REMOTE HOST ADDRESS\n");
		sock_close( &monit_local_sock );
		sock_close( &monit_remote_sock );
		return ERR_SOCK_BIND_HOST;
	}

	//Create tick thread
	if ( tc_thread_create( tc_server_monit_tick_thread, &tick_thread_id, &quit, &tick_lock, 100 ) ){
		fprintf(stderr,"tc_server_monitoring_init() : ERROR CREATING MONITORING TICK ENTRIES THREAD\n");
		sock_close( &monit_local_sock );
		sock_close( &monit_remote_sock );
		return ERR_THREAD_CREATE;
	}

	//Create tock thread
	if ( tc_thread_create( tc_server_monit_tock_thread, &tock_thread_id, &quit, &tock_lock, 100 ) ){
		fprintf(stderr,"tc_server_monitoring_init() : ERROR CREATING MONITORING TOCK ENTRIES THREAD\n");
		sock_close( &monit_local_sock );
		sock_close( &monit_remote_sock );
		return ERR_THREAD_CREATE;
	}

	init = 1;
	
	DEBUG_MSG_SERVER_MONIT("tc_server_monitoring_init() Monitoring module started\n");

	return ERR_OK;
}

int tc_server_monitoring_close( void )
{
	DEBUG_MSG_SERVER_MONIT("tc_server_monitoring_close() ...\n");

	if ( !init ){
		fprintf(stderr,"tc_server_monitoring_close() : MODULE ISNT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	//Stop tock thread
	if ( tc_thread_destroy( &tock_thread_id, &quit, &tock_lock, 100 ) ){
		fprintf(stderr,"tc_server_monitoring_close() : ERROR DESTROYING TOCK ENTRIES THREAD\n");
		return ERR_THREAD_DESTROY;
	}

	//Stop tick thread
	if ( tc_thread_destroy( &tick_thread_id, &quit, &tick_lock, 100 ) ){
		fprintf(stderr,"tc_server_monitoring_close() : ERROR DESTROYING TICK ENTRIES THREAD\n");
		return ERR_THREAD_DESTROY;
	}

	//Close local socket
	if ( sock_close ( &monit_local_sock ) ){
		fprintf(stderr,"tc_server_monitoring_close() : ERROR CLOSING LOCAL MONITORING SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	//Close remote socket
	if ( sock_close( &monit_remote_sock ) ){
		fprintf(stderr,"tc_server_monitoring_close() : ERROR CLOSING REMOTE MONITORING SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	init = 0;

	DEBUG_MSG_SERVER_MONIT("tc_server_monitoring_close() Monitoring module closed\n");

	return ERR_OK;
}

static int tc_server_monit_tick( unsigned int node_id )
{
	DEBUG_MSG_SERVER_MONIT("tc_server_monit_tick() Node Id %u ...\n",node_id);

	NODE_ENTRY *node = NULL; 

	if ( !init ){
		fprintf(stderr,"tc_server_monit_tick() : MODULE ISNT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	//Lock database
	tc_server_db_lock();

	//Get node entry
	if ( !(node = tc_server_db_node_search( node_id )) ){
		fprintf(stderr,"tc_server_monit_tick() : NODE ID %u NOT REGISTERED\n",node_id);
		tc_server_db_unlock();
		return ERR_NODE_NOT_REG;
	}

	node->heartbeat = HEARBEAT_COUNT;

	//Unlock database
	tc_server_db_unlock();

	DEBUG_MSG_SERVER_MONIT("tc_server_monit_tick() Node Id %u heartbeat counter reseted\n",node->node_id);

	return ERR_OK;
}

static int tc_server_monit_tock( void )
{
	DEBUG_MSG_SERVER_MONIT("tc_server_monit_tock() ...\n");

	NODE_ENTRY *node = NULL, *aux = NULL; 

	if ( !init ){
		fprintf(stderr,"tc_server_monit_tock() : MODULE ISNT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	//Lock database
	tc_server_db_lock();

	//For all node entries in database decrement heartbeat counter and check for dead nodes
	node = tc_server_db_node_get_first();
	
	while ( node ){
		if ( --node->heartbeat < 0 ){
			fprintf(stderr,"tc_server_monit_tock() : NODE ID %u DIED -- REMOVING IT\n",node->node_id);
			//Send notification
			tc_server_notifications_send_node_event( EVENT_NODE_UNPLUG, node );
			aux = node->next;
			tc_server_management_rm_node( node );
			node = aux;
			continue;
		}
		node = node->next;
	}  

	//Unlock database
	tc_server_db_unlock();
		
	DEBUG_MSG_SERVER_MONIT("tc_server_monit_tock() Decremented all nodes heartbeart counter\n");

	return ERR_OK;
}

static void tc_server_monit_tock_thread( void )
{
	DEBUG_MSG_SERVER_MONIT("tc_server_monit_tock_thread() ...\n");

	pthread_mutex_lock( &tock_lock );

	while ( quit == THREAD_RUN ){

		tc_server_monit_tock();
		usleep(HEARTBEAT_DEC_PERIOD);
	}

	pthread_mutex_unlock( &tock_lock );

	DEBUG_MSG_SERVER_MONIT("tc_server_monit_tock_thread() Monitoring tock entries thread ending\n");

	pthread_exit(NULL);
}

static void tc_server_monit_tick_thread( void )
{
	DEBUG_MSG_SERVER_MONIT("tc_server_monit_tick_thread() ...\n");

	struct timeval timeout;
	fd_set fds;
	int highest_fd;

	NET_MSG request;
	NET_ADDR client;

	pthread_mutex_lock( &tick_lock );

	//Get highest socket fd
	highest_fd = monit_remote_sock.fd;
	if ( monit_local_sock.fd > monit_remote_sock.fd )
		highest_fd = monit_local_sock.fd;

	while ( quit == THREAD_RUN ){

		//Prepare timed-out receive
		FD_ZERO(&fds);
		FD_SET(monit_remote_sock.fd, &fds);
		FD_SET(monit_local_sock.fd, &fds);

		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;

		if ( select(highest_fd+1, &fds, 0, 0, &timeout) <= 0)
			continue;

		if ( FD_ISSET(monit_local_sock.fd, &fds) ){
			//Client is in the same local node as server
			tc_network_get_msg( &monit_local_sock, 0, &request, &client );

		}else{
			//Client is in a remote node	
			tc_network_get_msg( &monit_remote_sock, 0, &request, &client );

		}

		DEBUG_MSG_SERVER_MONIT("tc_server_monit_tick_thread() : Received request %c from client %s:%u\n",request.op,client.name_ip,client.port);
		DEBUG_MSG_SERVER_MONIT("tc_server_monit_tick_thread() : Node Id %u\n",request.node_ids[0]);

		if ( (request.type == REQ_MSG) && (request.op == HEART_SIG) )
			tc_server_monit_tick( request.node_ids[0] );
		else
			fprintf(stderr,"tc_server_monit_tick_thread() : INVALID OPERATION REQUEST FROM NODE ID %u\n",request.node_ids[0]);
	}

	pthread_mutex_unlock( &tick_lock );

	DEBUG_MSG_SERVER_MONIT("tc_server_monit_tick_thread() Monitoring tick entries thread ending\n");

	pthread_exit(NULL);
}
