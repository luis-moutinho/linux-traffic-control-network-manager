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

/**	@file TC_Client_Management.c
*	@brief Source code of the functions for the client management module
*
*	This file contains the implementation of the functions for the client management
*	module. This module receives several types of requests from the server and handles them (I.E reservations requests,unbind requests, topic updates.. ).
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

#include "TC_Client_Reserv.h"
#include "TC_Client_DB.h"
#include "TC_Error_Types.h"
#include "Sockets.h"
#include "TC_Data_Types.h"
#include "TC_Utils.h"
#include "TC_Config.h"

/** 	@def DEBUG_MSG_CLIENT_MANAGEMENT
*	@brief If "ENABLE_DEBUG_CLIENT_MANAGEMENT" is defined debug messages related to this module are printed
*/
#if ENABLE_DEBUG_CLIENT_MANAGEMENT
#define DEBUG_MSG_CLIENT_MANAGEMENT(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG_CLIENT_MANAGEMENT(...)
#endif

static char init = 0;
static char quit = 0;

static pthread_t manag_thread_id;
static pthread_mutex_t manag_lock;

static NET_ADDR server_addr;
static char local_ip[20];

//Socket from where to receive server requests
static SOCK_ENTITY req_sock;

//Socket where to send requests reply message
static SOCK_ENTITY ans_sock;

static unsigned int manag_node_id = 0;

static void management_handler( void );
static int tc_client_management_open_remote_sock( void );
static int tc_client_management_open_local_sock( void );
static int tc_client_management_close_sock( void );

int tc_client_management_init( char *ifface, unsigned int node_id, NET_ADDR *server )
{
	DEBUG_MSG_CLIENT_MANAGEMENT("tc_client_management_init() ...\n");

	if ( init ){
		fprintf(stderr,"tc_client_management_init() : MODULE ALREADY INITIALIZED\n");
		return ERR_C_ALREADY_INIT;
	}

	assert( ifface );
	assert( node_id );
	assert( server );

	//Get nic ip address
	if ( tc_network_get_nic_ip( ifface, local_ip ) < 0){
		fprintf(stderr,"tc_client_management_init() : ERROR GETTING NIC IP ADDRESS\n");
		return ERR_INVALID_NIC;
	}

	//Set server address
	strcpy(server_addr.name_ip, server->name_ip);
	server_addr.port = server->port;

	if ( !server_addr.port ){
		//Server is in the same local node
		if ( tc_client_management_open_local_sock() ){
			fprintf(stderr,"tc_client_management_init() : ERROR CREATING LOCAL SOCKETS\n");
			return ERR_SOCK_CREATE;
		}
	}else{
		//Server in in a remote node
		if ( tc_client_management_open_remote_sock() ){
			fprintf(stderr,"tc_client_management_init() : ERROR CREATING REMOTE SOCKETS\n");
			return ERR_SOCK_CREATE;
		}
	}

	//Launch management thread
	if ( tc_thread_create( management_handler, &manag_thread_id, &quit, &manag_lock, 100 ) ){
		fprintf(stderr,"tc_client_management_init() : ERROR CREATING MANAGEMENT THREAD\n");
		tc_client_management_close_sock();
		return ERR_THREAD_CREATE;
	}

	manag_node_id = node_id;
	init = 1;
	
	DEBUG_MSG_CLIENT_MANAGEMENT("tc_client_management_init() Management module initialized\n");

	return ERR_OK;
}

int tc_client_management_close( void )
{
	DEBUG_MSG_CLIENT_MANAGEMENT("tc_client_management_close() ...\n");

	if ( !init ){
		fprintf(stderr,"tc_client_management_close() : MODULE ISNT RUNNING\n");
		return ERR_C_NOT_INIT;
	}

	//Stop monitoring thread
	if ( tc_thread_destroy( &manag_thread_id, &quit, &manag_lock, 100) ){
		fprintf(stderr,"tc_client_management_close() : ERROR DESTROYING MANAGEMENT THREAD\n");
		return ERR_THREAD_DESTROY;
	}

	//Close sockets
	if ( tc_client_management_close_sock() ){
		fprintf(stderr,"tc_client_managemente_close() : ERROR CLOSING SOCKETS\n");
		return ERR_SOCK_CLOSE;
	}

	init = 0;
	manag_node_id = 0;
	strcpy(local_ip,"");
	strcpy(server_addr.name_ip,"");
	server_addr.port = 0;

	DEBUG_MSG_CLIENT_MANAGEMENT("tc_client_management_close() Management module closed\n");

	return ERR_OK;
}

static void management_handler( void )
{
	DEBUG_MSG_CLIENT_MANAGEMENT("management_handler() ...\n");

	int i;
	char aux_tx,aux_rx;
	TOPIC_C_ENTRY *topic = NULL;
	NET_MSG msg,ans;

	pthread_mutex_lock( &manag_lock );

	while ( quit == THREAD_RUN ){

		//Poll until one request is received
				
		while ( !quit && tc_network_get_msg( &req_sock, 100000, &msg, NULL ) );

		//Check if it is a valid request
		if ( msg.type != REQ_MSG ){
			fprintf(stderr,"management_handler() : INVALID REQUEST MESSAGE -- GOING TO DISCARD IT\n");
			continue;
		}

		//Check if received message requests an operation in this node
		for ( i = 0; i < msg.n_nodes; i++ ){
			if( msg.node_ids[i] == manag_node_id )
				//Found request for this node
				break;
		}

		if ( i >= msg.n_nodes ){
			//No request for this node
			printf("management_handler() : Going to discard request( not for this node )\n");
			continue;
		}
 
		DEBUG_MSG_CLIENT_MANAGEMENT("management_handler() : Operation requested by server : "); if( ENABLE_DEBUG_CLIENT_MANAGEMENT ) tc_op_type_print( msg.op);
		DEBUG_MSG_CLIENT_MANAGEMENT("management_handler() : Topic Id %u Node Id %u\n",msg.topic_id,msg.node_ids[0]);

		//Prepare answer msg
		memset(&ans,0,sizeof(NET_MSG));

		ans.type = ANS_MSG;
		ans.op = REQ_ACCEPTED;
		ans.error = ERR_OK;
		ans.node_ids[0] = manag_node_id;
		ans.n_nodes = 1;
		ans.topic_id = msg.topic_id;
	
		//Lock topic database
		tc_client_db_lock();

		switch ( msg.op ){

			case BIND_TX :
		
				topic = tc_client_db_topic_search(msg.topic_id);

				//Check if its registered as producer of this topic 
				if ( !topic || !(topic->is_producer) ){
					fprintf(stderr,"management_handler() : NOT REGISTERED AS PRODUCER OF TOPIC ID %u\n",msg.topic_id);
					ans.op = REQ_REFUSED;
					ans.error = ERR_NODE_NOT_REG_TX;
					break;
				}

				//Bind to topic as producer
				topic->is_tx_bound = 1;

				printf("management_handler() : Bound as producer of topic id %u\n",topic->topic_id);
				break;

			case BIND_RX :

				topic = tc_client_db_topic_search(msg.topic_id);

				//Check if its registered as consumer of this topic 
				if ( !topic || !(topic->is_consumer) ){
					fprintf(stderr,"management_handler() : NOT REGISTERED AS CONSUMER OF TOPIC ID %u\n",msg.topic_id);
					ans.op = REQ_REFUSED;
					ans.error = ERR_NODE_NOT_REG_RX;
					break;
				}

				//Bind to topic as consumer
				topic->is_rx_bound = 1;

				printf("management_handler() : Bound as consumer of topic id %u\n",topic->topic_id);
				break;

			case DEL_TOPIC :

				//Check if node has topic entry
				topic = tc_client_db_topic_search(msg.topic_id);

				if ( !topic ){
					fprintf(stderr,"management_handler() : Topic id %u doesnt exist -- Ignoring removal request\n",msg.topic_id);
					break;
				}

				//Remove topic entry (save state if something fails)
				//NOTE : some send/receive calls might have been blocked after we unbound node...unlock topics before destroying them
				//Signal that topic is being destroyed
				topic->is_closing = 1;
				aux_tx = topic->is_tx_bound;
				aux_rx = topic->is_rx_bound;
				topic->is_tx_bound = 0;
				topic->is_rx_bound = 0;

				//Unblock receive call
				sock_send( &topic->unblock_rx_sock, &topic->unblock_rx_sock.host, "0", 5);

				//Safeguard timeout
				usleep(1000);

				//Free reservation
				if ( topic->is_producer ){
					if ( tc_client_reserv_del( msg.topic_id, &msg.topic_addr, msg.topic_load ) ){
						fprintf(stderr,"management_handler() : ERROR FREEING RESERVATION\n");
						topic->is_tx_bound = aux_tx;
						topic->is_rx_bound = aux_rx;
						topic->is_closing = 0;
						ans.op = REQ_REFUSED;
						ans.error = ERR_RESERV_DEL;
						break;
					 }
				}

				//Close topic socket
				if ( sock_close( &topic->topic_sock ) ){
					fprintf(stderr,"management_handler() : ERROR CLOSING TOPIC ID %u SOCKET\n",msg.topic_id);
					topic->is_tx_bound = aux_tx;
					topic->is_rx_bound = aux_rx;
					topic->is_closing = 0;
					ans.op = REQ_REFUSED;
					ans.error = ERR_SOCK_CLOSE;
					break;
				}

				tc_client_db_topic_delete( topic );
				
				printf("management_handler() : Topic id %u destroyed\n",msg.topic_id);
				break;

			case SET_TOPIC_PROP :

				//Check if node has topic entry
				topic = tc_client_db_topic_search(msg.topic_id);

				if ( !topic ){
					fprintf(stderr,"management_handler() : Topic id %u doesnt exist -- Ignoring modify request\n",msg.topic_id);
					break;
				}
				
				//Update reservation
				if ( topic->is_producer ){
					if ( tc_client_reserv_set( msg.topic_id, &msg.topic_addr, msg.topic_load ) ){
						fprintf(stderr,"management_handler() : ERROR UPDATING RESERVATION\n");
						ans.op = REQ_REFUSED;
						ans.error = ERR_RESERV_SET;
						break;
					 }
				}

				//Update topic properties
				topic->channel_size = msg.channel_size;
				topic->channel_period = msg.channel_period;
				
				printf("management_handler() : Updated topic id %u properties\n",msg.topic_id);
				break;

			case UNBIND_TX :

				//Check if node is registered as producer of topic
				topic = tc_client_db_topic_search(msg.topic_id);

				if ( !topic || !topic->is_producer ){
					fprintf(stderr,"management_handler() : NOT REGISTERED AS PRODUCER OF TOPIC ID %u\n",msg.topic_id);
					ans.op = REQ_REFUSED;
					ans.error = ERR_NODE_NOT_REG_TX;
					break;
				}

				//Unbind node from topic as producer
				topic->is_tx_bound = 0;
	
				printf("management_handler() : Unbound as producer from topic id %u\n",msg.topic_id);
				break;

			case UNBIND_RX :

				//Check if node is registered as consumer of topic
				topic = tc_client_db_topic_search(msg.topic_id);

				if ( !topic || !topic->is_consumer ){
					fprintf(stderr,"management_handler() : NOT REGISTERED AS CONSUMER OF TOPIC ID %u\n",msg.topic_id);
					ans.op = REQ_REFUSED;
					ans.error = ERR_NODE_NOT_REG_RX;
					break;
				}

				//Unbind node from topic as consumer
				topic->is_rx_bound = 0;

				//Flag as not bound as rx and unblock receive call
				sock_send( &topic->unblock_rx_sock, &topic->unblock_rx_sock.host, "0", 5);

				printf("management_handler() : Unbound as consumer from topic id %u\n",msg.topic_id);
				break;

			case TC_RESERV :

				if ( tc_client_reserv_add( msg.topic_id, &msg.topic_addr, msg.topic_load ) ){
					fprintf(stderr,"management_handler() : ERROR CREATING NEW RESERVATION\n");
					ans.op = REQ_REFUSED;
					ans.error = ERR_RESERV_ADD;
					break;
				}
				
				printf("management_handler() : Reservation ( load %u ) for topic id %u created\n",msg.topic_load,msg.topic_id);
				break;

			case TC_MODIFY :

				if ( tc_client_reserv_set( msg.topic_id, &msg.topic_addr, msg.topic_load ) ){
					fprintf(stderr,"management_handler() : ERROR MODIFYING RESERVATION\n");
					ans.op = REQ_REFUSED;
					ans.error = ERR_RESERV_SET;
					break;
				}

				printf("management_handler() : Reservation topic id %u updated ( new load %u )\n",msg.topic_id,msg.topic_load);
				break;

			case TC_FREE :

				if ( tc_client_reserv_del( msg.topic_id, &msg.topic_addr, msg.topic_load ) ){
					fprintf(stderr,"management_handler() : ERROR FREEING RESERVATION\n");
					ans.op = REQ_REFUSED;
					ans.error = ERR_RESERV_DEL;
					break;
				}

				printf("management_handler() : Reservation (load %u ) of topic id %u rfreed \n",msg.topic_load,msg.topic_id);
				break;

			default :
				fprintf(stderr,"management_handler() : INVALID MANAGEMENT OPERATION\n");
				ans.op = REQ_REFUSED;
				ans.error = ERR_INVALID_PARAM;
				break;
		}

		DEBUG_MSG_CLIENT_MANAGEMENT("management_handler() Going to send answer %c\n",ans.error);

		tc_network_send_msg( &ans_sock, &ans, NULL );

		//Unlock topic database
		tc_client_db_unlock();
	}

	pthread_mutex_unlock( &manag_lock );

	DEBUG_MSG_CLIENT_MANAGEMENT("management_handler() Management thread ending\n");

	pthread_exit(NULL);
}

static int tc_client_management_open_local_sock( void )
{
	DEBUG_MSG_CLIENT_MANAGEMENT("tc_client_management_open_local_sock() ...\n");

	NET_ADDR host,peer;

	//Create local server requests socket
	if ( sock_open( &req_sock, LOCAL ) < 0){
		fprintf(stderr,"tc_client_management_open_local_sock() : ERROR CREATING LOCAL SERVER REQUEST SOCKET\n");
		return ERR_SOCK_CREATE;
	}

	//Set and bind to host address
	strcpy(host.name_ip, CLIENT_MANAGEMENT_REQ_LOCAL_FILE);
	host.port = 0;

	if ( sock_bind( &req_sock, &host ) ){
		fprintf(stderr,"tc_client_management_open_local_sock() : ERROR BINDING REQUEST SOCKET TO HOST ADDRESS\n");
		sock_close(&req_sock);
		return ERR_SOCK_BIND_HOST;
	}

	//Connect socket to server
	strcpy(peer.name_ip, SERVER_MANAGEMENT_REQ_LOCAL_FILE);
	peer.port = 0;

	if ( sock_connect_peer( &req_sock, &peer ) ){
		fprintf(stderr,"tc_client_management_open_local_sock() : ERROR CONNECTING REQUEST LOCAL SOCKET TO SERVER\n");
		sock_close ( &req_sock );
		return ERR_SOCK_BIND_PEER;
	}


	//Create local server reply socket
	if ( sock_open( &ans_sock, LOCAL ) < 0){
		fprintf(stderr,"tc_client_management_open_local_sock() : ERROR CREATING LOCAL SERVER REPLY SOCKET\n");
		return ERR_SOCK_CREATE;
	}

	//Set and bind to host address
	strcpy(host.name_ip, CLIENT_MANAGEMENT_ANS_LOCAL_FILE);
	host.port = 0;

	if ( sock_bind( &ans_sock, &host ) ){
		fprintf(stderr,"tc_client_management_open_local_sock() : ERROR BINDING REPLY SOCKET TO HOST ADDRESS\n");
		sock_close(&ans_sock);
		return ERR_SOCK_BIND_HOST;
	}

	//Connect socket to server
	strcpy(peer.name_ip, SERVER_MANAGEMENT_ANS_LOCAL_FILE);
	peer.port = 0;

	if ( sock_connect_peer( &ans_sock, &peer ) ){
		fprintf(stderr,"tc_client_management_open_local_sock() : ERROR CONNECTING REPLY LOCAL SOCKET TO SERVER\n");
		sock_close ( &ans_sock );
		return ERR_SOCK_BIND_PEER;
	}

	DEBUG_MSG_CLIENT_MANAGEMENT("tc_client_management_open_local_sock() Local sockets created\n");

	return ERR_OK;
}

static int tc_client_management_open_remote_sock( void )
{
	DEBUG_MSG_CLIENT_MANAGEMENT("tc_client_management_open_remote_sock() ...\n");

	NET_ADDR host,peer;

	//Server is in a remote node

	//Create remote server request socket
	if ( sock_open( &req_sock, REMOTE_UDP_GROUP ) < 0){
		fprintf(stderr,"tc_client_management_open_remote_sock() : ERROR CREATING SERVER REQUEST SOCKET\n");
		return ERR_SOCK_CREATE;
	}

	//Set and bind to host address
	strcpy(host.name_ip,local_ip);
	host.port = MANAGEMENT_GROUP_PORT;

	if ( sock_bind( &req_sock, &host ) ){
		fprintf(stderr,"tc_client_management_open_remote_sock() : ERROR BINDING REQUEST SOCKET TO LOCAL ADDRESS\n");
		sock_close( &req_sock );
		return ERR_SOCK_BIND_HOST;
	}

	//Set remote address
	strcpy(peer.name_ip, MANAGEMENT_GROUP_IP);
	peer.port = MANAGEMENT_GROUP_PORT;	
	
	//Join discovery group
	if ( sock_connect_group_rx( &req_sock, &peer ) ){
		fprintf(stderr,"tc_client_management_open_remote_sock() : ERROR REGISTERING TO MANAGEMENT GROUP\n");
		sock_close ( &req_sock );
		return ERR_SOCK_BIND_PEER;
	}


	//Create remote server reply socket
	if ( sock_open( &ans_sock, REMOTE_UDP ) < 0){
		fprintf(stderr,"tc_client_management_open_remote_sock() : ERROR CREATING SERVER REPLY SOCKET\n");
		return ERR_SOCK_CREATE;
	}

	//Set and bind to host address
	strcpy(host.name_ip,local_ip);
	host.port = server_addr.port + MANAGEMENT_PORT_OFFSET;

	if ( sock_bind( &ans_sock, &host ) ){
		fprintf(stderr,"tc_client_management_open_remote_sock() : ERROR BINDING REPLY SOCKET TO LOCAL ADDRESS\n");
		sock_close( &ans_sock );
		return ERR_SOCK_BIND_HOST;
	}

	//Set remote address
	strcpy(peer.name_ip, server_addr.name_ip);
	peer.port = server_addr.port + MANAGEMENT_PORT_OFFSET;

	//Connect to server
	if ( sock_connect_peer( &ans_sock, &peer ) ){
		fprintf(stderr,"tc_client_management_open_remote_sock() : ERROR CONNECTING REPLY SOCKET TO SERVER\n");
		sock_close( &ans_sock );
		return ERR_SOCK_BIND_PEER;
	}

	DEBUG_MSG_CLIENT_MANAGEMENT("tc_client_management_open_remote_sock() Remote sockets created\n");

	return ERR_OK;
}

static int tc_client_management_close_sock( void )
{
	DEBUG_MSG_CLIENT_MANAGEMENT("tc_client_management_close_sock() ...\n");

	if ( sock_close( &req_sock ) ){
		fprintf(stderr,"tc_client_management_close_sock() : ERROR CLOSING REQUEST SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	if ( sock_close( &ans_sock ) ){
		fprintf(stderr,"tc_client_management_close_sock() : ERROR CLOSING REPLY SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	DEBUG_MSG_CLIENT_MANAGEMENT("tc_client_management_close_sock() Sockets closed\n");

	return ERR_OK;
}

