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

/**	@file TC_Server.c
*	@brief Source code of the functions for the server side API
*
*	This file contains the implementation of the functions for the server API
*	to be used by the application. This module creates the necessary sockets and threads to receive and handle requests from clients.
*	This module also initializes all the necessary internal control modules necessary to resolve the requests. Top module
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

#include "TC_Server.h"
#include "TC_Server_DB.h"
#include "TC_Server_AC.h"
#include "TC_Server_Monitoring.h"
#include "TC_Server_Management.h"
#include "TC_Server_Discovery.h"
#include "TC_Server_Notifications.h"

#include "TC_Data_Types.h"
#include "TC_Utils.h"
#include "TC_Error_Types.h"
#include "TC_Config.h"

/**	@def DEBUG_MSG_TC_SERVER
*	@brief If "ENABLE_DEBUG_SERVER" is defined debug messages related to this module are printed
*/
#if ENABLE_DEBUG_SERVER
#define DEBUG_MSG_TC_SERVER(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG_TC_SERVER(...)
#endif

static char init = 0;
static char quit = 0;

static char nic_ip[50];

static NET_ADDR server_remote, server_local;
static SOCK_ENTITY remote_sock,local_sock;

static pthread_t server_thread_id;
static pthread_mutex_t server_lock;

static int tc_server_comm_init( void );
static int tc_server_comm_close( void );
static int tc_server_modules_init( void );
static int tc_server_modules_close( void );

static void tc_server_req_get( void );
static void tc_server_req_resolve( SOCK_ENTITY *sock );

int tc_server_init( char *ifface, unsigned int server_port )
{
	DEBUG_MSG_TC_SERVER("tc_server_init() ...\n");

	int ret;

	if ( init ){
		fprintf(stderr,"tc_server_init() : SERVER ALREADY RUNNING\n");
		return ERR_S_ALREADY_INIT;
	}

	//Validate parameters
	if( !ifface || !server_port ){
		fprintf(stderr,"tc_server_init() : INVALID PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Get nic ip address
	if ( tc_network_get_nic_ip( ifface, nic_ip ) ){
		fprintf(stderr,"tc_server_init() : ERROR GETTING NIC IP ADDRESS\n");
		return ERR_INVALID_NIC;
	}

	//Set server remote address
	strcpy(server_remote.name_ip,nic_ip);
	server_remote.port = server_port;

	//Set server local address
	strcpy(server_local.name_ip, SERVER_AC_LOCAL_FILE);
	server_local.port = 0;

	if ( (ret = tc_server_modules_init()) ){
		fprintf(stderr,"tc_server_init() : ERROR INITIALIZING SERVER INTERNAL MODULES\n");
		return ret;
	}
	
	//Create requests polling thread
	if ( tc_thread_create( tc_server_req_get, &server_thread_id, &quit, &server_lock, 100 ) ){
		fprintf(stderr,"tc_server_init() : ERROR CREATING REQUESTS POLLING THREAD\n");
		tc_server_modules_close();
		return ERR_THREAD_CREATE;
	}
	
	init = 1;

	DEBUG_MSG_TC_SERVER("tc_server_init() Server is now running\n");

	return ERR_OK;
}

int tc_server_close( void )
{
	DEBUG_MSG_TC_SERVER("tc_server_close() ...\n");

	int ret;

	if ( !init ){
		fprintf(stderr,"tc_server_close() : SERVER IS NOT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	//Stop requests polling thread
	if ( tc_thread_destroy( &server_thread_id, &quit, &server_lock, 100) ){
		fprintf(stderr,"tc_server_close() : ERROR DESTROYING REQUESTS POLLING THREAD\n");
		return ERR_THREAD_DESTROY;
	}

	//Close modules
	if ( (ret = tc_server_modules_close()) ){
		fprintf(stderr,"tc_server_close() : ERROR CLOSING SERVER INTERNAL MODULES\n");
		return ret;
	}

	strcpy(nic_ip,"");
	strcpy(server_remote.name_ip,"");
	strcpy(server_local.name_ip,"");
	server_remote.port = server_local.port = 0;

	init = 0;

	DEBUG_MSG_TC_SERVER("tc_server_close() Server is now closed\n");

	return ERR_OK;
}

static void tc_server_req_get( void )
{
	DEBUG_MSG_TC_SERVER("tc_server_req_get() ...\n");

	struct timeval timeout;
	fd_set fds;
	int highest_fd;
	
	pthread_mutex_lock( &server_lock );

	//Get highest socket fd
	highest_fd = remote_sock.fd;
	if ( local_sock.fd > remote_sock.fd )
		highest_fd = local_sock.fd;

	while ( !quit ){
	
		//Prepare timed-out receive
		FD_ZERO(&fds);
		FD_SET(remote_sock.fd, &fds);
		FD_SET(local_sock.fd, &fds);

		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;

		if ( select(highest_fd+1, &fds, 0, 0, &timeout) <= 0)
			continue;

		if ( FD_ISSET(local_sock.fd, &fds) ){
			//Received request from a client that is in the same local node as server
			tc_server_req_resolve( &local_sock );
		}

		if ( FD_ISSET(remote_sock.fd, &fds) ){
			//Received request from a client that is in a remote node
			tc_server_req_resolve( &remote_sock );
		}
	}

	pthread_mutex_unlock( &server_lock );

	DEBUG_MSG_TC_SERVER("tc_server_req_get() Requests polling thread ending\n");

	pthread_exit(NULL);
}

static void tc_server_req_resolve( SOCK_ENTITY *sock )
{
	NET_ADDR client, topic_addr;
	NET_MSG req, ans;

	//if ( sock->type == REMOTE_UDP ) printf("\nRECEIVING REQUEST FROM REMOTE\n");
	//if ( sock->type == LOCAL )  printf("\nRECEIVING REQUEST FROM LOCAL\n");

	tc_network_get_msg( sock, 0, &req, &client );

	//Check if it is a valid request
	if ( req.type != REQ_MSG ){
		fprintf(stderr,"tc_server_req_resolve() : INVALID MESSAGE TYPE -- GOING TO DISCARD\n");
		return;
	}

	printf("\ntc_server_req_resolve() : Received request from client %s:%u . Operation : ",client.name_ip,client.port);
	tc_op_type_print( req.op );
	printf("tc_server_req_resolve() : Node Id %u Topic Id %u Size %u Period %u\n",req.node_ids[0],req.topic_id,req.channel_size,req.channel_period);
	
	//Prepare answer msg
	memset(&ans,0,sizeof(NET_MSG));

	ans.type = ANS_MSG;
	ans.op = REQ_ACCEPTED;
	ans.error = ERR_OK;
	ans.node_ids[0] = req.node_ids[0];
	ans.n_nodes = req.n_nodes;
	ans.topic_id = req.topic_id;
	ans.channel_size = req.channel_size;
	ans.channel_period = req.channel_period;
	
	//Lock database
	tc_server_db_lock();

	switch( req.op ){

		case REG_NODE :

			//Register node in the network
			if ( ( ans.error = tc_server_ac_add_node( req.node_ids[0], &client, &ans.node_ids[0] )) )
				ans.op = REQ_REFUSED;	

			//Answer request
			tc_network_send_msg( sock, &ans, &client );
	
			break;

		case UNREG_NODE :

			//Unregister node in the network
			if ( ( ans.error = tc_server_ac_rm_node( req.node_ids[0] )) )
				ans.op = REQ_REFUSED;	

			//Answer request
			tc_network_send_msg( sock, &ans, &client );
	
			break;

		case REG_TOPIC :

			//Register topic
			if ( ( ans.error = tc_server_ac_add_topic(  req.topic_id, req.channel_size, req.channel_period )) )
				ans.op = REQ_REFUSED;

			//Get registered topic properties (cross-check)
			if ( !( ans.error = tc_server_ac_get_topic_prop( req.topic_id, &(ans.topic_load), &(ans.channel_size), &(ans.channel_period), &topic_addr )) ){
				strcpy( ans.topic_addr.name_ip, topic_addr.name_ip );
				ans.topic_addr.port = topic_addr.port; 
			}else{
				ans.op = REQ_REFUSED;
			}

			//Answer request
			tc_network_send_msg( sock, &ans, &client );
	
			break;

		case DEL_TOPIC :

			//Remove topic from network (this also removes all bound/registered nodes from that topic )
			if ( ( ans.error = tc_server_ac_rm_topic(  req.topic_id )) )
				ans.op = REQ_REFUSED;
				
			//Answer request
			tc_network_send_msg( sock, &ans, &client );
			tc_server_db_topic_print();
			tc_server_db_node_print();
			break;

		case GET_TOPIC_PROP :

			//Get topic properties
			if ( !( ans.error = tc_server_ac_get_topic_prop( req.topic_id, &(ans.topic_load), &(ans.channel_size), &(ans.channel_period), &topic_addr )) ){
				strcpy( ans.topic_addr.name_ip, topic_addr.name_ip );
				ans.topic_addr.port = topic_addr.port; 
			}else{
				ans.op = REQ_REFUSED;
			}
			
			//Answer request
			tc_network_send_msg( sock, &ans, &client );
	
			break;

		case SET_TOPIC_PROP :

			//Set topic properties
			if ( ( ans.error = tc_server_ac_set_topic_prop( req.topic_id, req.channel_size, req.channel_period )) )
				ans.op = REQ_REFUSED;
			
			//Answere request
			tc_network_send_msg( sock, &ans, &client );

			break;

		case REG_PROD :

			//Register node as producer of topic
			if ( ( ans.error = tc_server_ac_add_prod( req.topic_id, req.node_ids[0] )) )
				ans.op = REQ_REFUSED;

			//Get topic properties
			if ( !( ans.error = tc_server_ac_get_topic_prop( req.topic_id, &(ans.topic_load), &(ans.channel_size), &(ans.channel_period), &topic_addr )) ){
				strcpy( ans.topic_addr.name_ip, topic_addr.name_ip );
				ans.topic_addr.port = topic_addr.port; 
			}else{
				ans.op = REQ_REFUSED;
			}

			//Answer request
			tc_network_send_msg( sock, &ans, &client );

			break;

		case UNREG_PROD :

			//Unregister node as producer of topic
			if ( (ans.error = tc_server_ac_rm_prod( req.topic_id, req.node_ids[0] )) )
				ans.op = REQ_REFUSED;

			//Answer request
			tc_network_send_msg( sock, &ans, &client );

			//Check for nodes without valid topic producers and unbind them
			tc_server_ac_check_topic_unbind( req.topic_id );	

			break;

		case REG_CONS :

			//Register node as consumer of topic
			if ( ( ans.error = tc_server_ac_add_cons( req.topic_id, req.node_ids[0] )) )
				ans.op = REQ_REFUSED;

			//Get topic properties
			if ( !( ans.error = tc_server_ac_get_topic_prop( req.topic_id, &(ans.topic_load), &(ans.channel_size), &(ans.channel_period), &topic_addr )) ){
				strcpy( ans.topic_addr.name_ip, topic_addr.name_ip );
				ans.topic_addr.port = topic_addr.port; 
			}else{
				ans.op = REQ_REFUSED;
			}
			
			//Answer request
			tc_network_send_msg( sock, &ans, &client );
	
			break;

		case UNREG_CONS :

			//Unregister node as consumer of topic
			if ( ( ans.error = tc_server_ac_rm_cons( req.topic_id, req.node_ids[0] )) )
				ans.op = REQ_REFUSED;

			//Answer request
			tc_network_send_msg( sock, &ans, &client );

			//Check for nodes without valid topic consumers and unbind them
			tc_server_ac_check_topic_unbind( req.topic_id );	

			break;

		case BIND_TX :

			//Set node bind request as producer of topic 
			if ( ( ans.error = tc_server_ac_bind_tx ( req.topic_id, req.node_ids[0] )) )
				ans.op = REQ_REFUSED;

			//Answer request
			tc_network_send_msg( sock, &ans, &client );	

			//Check for nodes with pending bind requests and bind them if possible
			tc_server_ac_check_topic_bind( req.topic_id );

			break;

		case UNBIND_TX :

			//Unbind producer node from topic
			if ( ( ans.error = tc_server_ac_unbind_tx ( req.topic_id, req.node_ids[0] )) )
				ans.op = REQ_REFUSED;

			//Answer request
			tc_network_send_msg( sock, &ans, &client );	

			//Check for nodes without valid topic producers and unbind them
			tc_server_ac_check_topic_unbind( req.topic_id );

			break;

		case BIND_RX :

			//Set node bind request as consumer of topic 
			if ( ( ans.error = tc_server_ac_bind_rx ( req.topic_id, req.node_ids[0] )) )
				ans.op = REQ_REFUSED;

			//Answer request
			tc_network_send_msg( sock, &ans, &client );	

			//Check for nodes with pending bind requests and bind them if possible
			tc_server_ac_check_topic_bind( req.topic_id );

			break;

		case UNBIND_RX :

			//Unbind consumer node from topic
			if ( ( ans.error = tc_server_ac_unbind_rx ( req.topic_id, req.node_ids[0] )) )
				ans.op = REQ_REFUSED;

			//Answer request
			tc_network_send_msg( sock, &ans, &client );	

			//Check for nodes without valid topic consumers and unbind them
			tc_server_ac_check_topic_unbind( req.topic_id );

			break;

		default :

			ans.op = REQ_REFUSED;
			ans.node_ids[0] = req.node_ids[0];

			tc_network_send_msg( sock, &ans, &client );
			break;
	}
	
	//Unlock database
	tc_server_db_unlock();

	return;
}

static int tc_server_comm_init( void )
{
	DEBUG_MSG_TC_SERVER("tc_server_comm_init() ...\n");

	//Create local server socket
	if ( sock_open( &local_sock, LOCAL ) < 0){
		fprintf(stderr,"tc_server_comm_init() : ERROR CREATING LOCAL SERVER SOCKET\n");
		return ERR_SOCK_CREATE;
	}

	//Set and bind to server local address
	if ( sock_bind( &local_sock, &server_local ) < 0 ){
		fprintf(stderr,"tc_server_comm_init() : ERROR BINDING SOCKET TO HOST ADDRESS\n");
		sock_close(&local_sock);
		return ERR_SOCK_BIND_HOST;
	}

	//Create remote server socket
	if ( sock_open( &remote_sock, REMOTE_UDP ) < 0){
		fprintf(stderr,"tc_server_comm_init() : ERROR CREATING SERVER SOCKET\n");
		sock_close( &local_sock );
		return ERR_SOCK_CREATE;
	}

	//Bind to server remote address
	if ( sock_bind( &remote_sock, &server_remote ) < 0){
		fprintf(stderr,"tc_server_comm_init() : ERROR BINDING SOCKET TO LOCAL ADDRESS\n");
		sock_close( &local_sock );
		sock_close( &remote_sock );
		return ERR_SOCK_BIND_HOST;
	}

	DEBUG_MSG_TC_SERVER("tc_server_comm_init() Created links\n");

	return ERR_OK;
}

static int tc_server_comm_close( void )
{
	DEBUG_MSG_TC_SERVER("tc_server_comm_close() ...\n");

	if ( sock_close ( &local_sock ) ){
		fprintf(stderr,"tc_server_comm_close() : ERROR CLOSING LOCAL SERVER SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	if ( sock_close( &remote_sock ) ){
		fprintf(stderr,"tc_server_comm_close() : ERROR CLOSING REMOTE SERVER SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	DEBUG_MSG_TC_SERVER("tc_server_comm_close() Closed links\n");

	return ERR_OK;
}

static int tc_server_modules_init( void )
{
	DEBUG_MSG_TC_SERVER("tc_server_modules_init() ...\n");

	//Create comunication links for client requests
	if ( tc_server_comm_init() ){
		fprintf(stderr,"tc_server_modules_init() : ERROR SETTING UP COMUNICATIONS\n");
		return ERR_COMM_INIT;
	}

	//Start server database module
	if ( tc_server_db_init() ){
		fprintf(stderr,"tc_server_modules_init() : ERROR STARTING DATABASE MODULE\n");
		tc_server_modules_close();
		return ERR_DB_INIT;
	}

	//Start admission control module
	if ( tc_server_ac_init() ){
		fprintf(stderr,"tc_server_modules_init() : ERROR STARTING ADMISSION CONTROL MODULE\n");	
		tc_server_modules_close();
		return ERR_AC_INIT;
	}

	//Start monitoring module
	if ( tc_server_monitoring_init( &server_remote ) ){
		fprintf(stderr,"tc_server_modules_init() : ERROR STARTING MONITORING MODULE\n");
		tc_server_modules_close();
		return ERR_MONIT_INIT;
	}

	//Start management module
	if ( tc_server_management_init( &server_remote ) ){
		fprintf(stderr,"tc_server_modules_init() : ERROR STARTING MANAGEMENT MODULE\n");
		tc_server_modules_close();
		return ERR_MANAG_INIT;
	}

	//Start discovery module
	if ( tc_server_discovery_init( &server_remote ) ) {
		fprintf(stderr,"tc_server_modules_init() : ERROR STARTING DISCOVERY MODULE\n");
		tc_server_modules_close();
		return ERR_DISCOVERY_INIT;
	}

	//Start notification module
	if ( tc_server_notifications_init( &server_remote ) ) {
		fprintf(stderr,"tc_server_modules_init() : ERROR STARTING NOTIFICATIONS MODULE\n");
		tc_server_modules_close();
		return ERR_NOTIFIC_INIT;
	}
	
	DEBUG_MSG_TC_SERVER("tc_server_modules_init() Internal modules initialized\n");

	return ERR_OK;
}

static int tc_server_modules_close( void )
{
	DEBUG_MSG_TC_SERVER("tc_server_modules_close() ...\n");

	int ret;

	//Stop notifications module
	if ( (ret = tc_server_notifications_close()) && ret != ERR_S_NOT_INIT ){
		fprintf(stderr,"tc_server_close() : ERROR CLOSING NOTIFICATIONS MODULE\n");
		return ERR_NOTIFIC_CLOSE;
	}

	//Stop discovery module
	if ( (ret = tc_server_discovery_close()) && ret != ERR_S_NOT_INIT ){
		fprintf(stderr,"tc_server_close() : ERROR CLOSING DISCOVERY MODULE\n");
		return ERR_DISCOVERY_CLOSE;
	}

	//Stop management control module
	if ( (ret = tc_server_management_close()) && ret != ERR_S_NOT_INIT ){
		fprintf(stderr,"tc_server_close() : ERROR CLOSING MANAGEMENT CONTROL MODULE\n");
		return ERR_MANAG_CLOSE;
	}

	//Stop monitoring module
	if ( (ret = tc_server_monitoring_close()) && ret != ERR_S_NOT_INIT ){
		fprintf(stderr,"tc_server_close() : ERROR CLOSING MONITORING MODULE\n");
		return ERR_MONIT_CLOSE;
	}

	//Stop admission control module
	if ( (ret = tc_server_ac_close()) && ret != ERR_S_NOT_INIT ){
		fprintf(stderr,"tc_server_close() : ERROR CLOSING ADMISSION CONTROL MODULE\n");
		return ERR_AC_CLOSE;
	}

	//Stop topic database module
	if ( (ret = tc_server_db_close()) && ret != ERR_S_NOT_INIT ){
		fprintf(stderr,"tc_server_close() : ERROR CLOSING DATABASE MODULE\n");
		return ERR_DB_CLOSE;
	}
	
	//Close comunication links
	if ( tc_server_comm_close() ){
		fprintf(stderr,"tc_server_comm_close() : ERROR CLOSING COMUNICATION LINKS\n");
		return ERR_COMM_CLOSE;
	}

	DEBUG_MSG_TC_SERVER("tc_server_modules_close() Internal modules closed\n");

	return ERR_OK;
}
