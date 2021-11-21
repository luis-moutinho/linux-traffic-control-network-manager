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

/**	@file TC_Client.c
*	@brief Source code of the functions for the client side API
*
*	This file contains the implementation of the functions for the client API
*	to be used by the application. This module also initializes all the necessary internal control modules. Top module
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>

#include "Sockets.h"
#include "TC_Data_Types.h"
#include "TC_Utils.h"
#include "TC_Error_Types.h"
#include "TC_Config.h"

#include "TC_Client_DB.h"
#include "TC_Client.h"
#include "TC_Client_Monit.h"
#include "TC_Client_Reserv.h"
#include "TC_Client_Management.h"
#include "TC_Client_Discovery.h"
#include "TC_Client_Notifications.h"

/**	@def DEBUG_MSG_TC_CLIENT
*	@brief If "ENABLE_DEBUG_CLIENT" is defined debug messages related to this module are printed
*/
#if ENABLE_DEBUG_CLIENT
#define DEBUG_MSG_TC_CLIENT(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG_TC_CLIENT(...)
#endif

static char init = 0;
static unsigned int tc_node_id = 0;
static char nic_ip[50];
static char nic_ifface[50];

static NET_ADDR server;
static SOCK_ENTITY server_sock;

//Serialize requests to server to avoid one request receiving the answer for another made before by another thread in the same node
static pthread_mutex_t server_lock;

static int tc_client_get_server_access( void );
static int tc_client_release_server_access( void );
static int tc_client_comm_init( void );
static int tc_client_comm_close( void );
static int tc_client_modules_init( void );
static int tc_client_modules_close( void );
static int tc_client_node_reg ( unsigned int node_id, unsigned int *ret_node_id );
static int tc_client_node_unreg ( void );

int tc_client_init( char *ifface, unsigned int node_id )
{
	DEBUG_MSG_TC_CLIENT("tc_client_init() ...\n");

	int ret;

	if ( init ){
		fprintf(stderr,"tc_client_init() : MODULE ALREADY INITIALIZED\n");
		return ERR_C_ALREADY_INIT;
	}

	//Validate parameters
	if( !ifface ){
		fprintf(stderr,"tc_client_init() : INVALID PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Get nic ip address
	strcpy(nic_ifface,ifface);
	if ( tc_network_get_nic_ip( ifface, nic_ip ) ){
		fprintf(stderr,"tc_client_init() : ERROR GETTING NIC IP ADDRESS\n");
		return ERR_INVALID_NIC;
	}

	//Save requested node_id ( if 0 server will assign a random id )
	tc_node_id = node_id;

	//Start client modules
	if ( (ret = tc_client_modules_init()) ){
		fprintf(stderr,"tc_client_init() : ERROR INITIALIZING CLIENT INTERNAL MODULES\n");
		return ret;
	}
 
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
	pthread_mutex_init( &server_lock, &attr );

	init = 1;

	DEBUG_MSG_TC_CLIENT("tc_client_init() Client initialized\n");

	//Return assigned node_id
	return (int) tc_node_id;
}

int tc_client_close( void )
{
	DEBUG_MSG_TC_CLIENT("tc_client_close() ...\n");

	int ret;

	if ( !init ){
		fprintf(stderr,"tc_client_close() : MODULE IS NOT INITIALIZED\n");
		return ERR_C_NOT_INIT;
	}

	if ( (ret = tc_client_modules_close()) ){
		fprintf(stderr,"tc_client_close() : ERROR CLOSING INTERNAL CLIENT MODULES\n");
		return ret;
	}

	init = 0;
	tc_node_id = 0;
	pthread_mutex_destroy( &server_lock );
	strcpy( nic_ip, "" );
	strcpy( nic_ifface, "" );			
	memset(&server_sock,0,sizeof(SOCK_ENTITY));

	DEBUG_MSG_TC_CLIENT("tc_client_close() Client closed\n");

	return ERR_OK;
}

int tc_client_get_node_event( unsigned int timeout, unsigned char *ret_event, unsigned int *ret_node_id )
{
	DEBUG_MSG_TC_CLIENT("tc_client_get_node_event() ...\n");

	NET_MSG	msg;

	if ( !init ){
		fprintf(stderr,"tc_client_get_node_event() : MODULE IS NOT INITIALIZED\n");
		return ERR_C_NOT_INIT;
	}	

	//Validate parameters
	if ( !ret_event || !ret_node_id ){
		fprintf(stderr,"tc_client_get_node_event() : INVALID PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	if ( tc_client_notifications_get( timeout, &msg ) ){
		DEBUG_MSG_TC_CLIENT("tc_client_get_node_even() : COULD NOT GET A NOTIFICATION MESSAGE\n");
		return ERR_DATA_RECEIVE;
	}

	if ( msg.event == EVENT_NODE_PLUG )
		*ret_event = NODE_PLUG;
	else
		*ret_event = NODE_UNPLUG;
	
	*ret_node_id = msg.node_ids[0];
	
	DEBUG_MSG_TC_CLIENT("tc_client_get_node_event() Got event %d on node id %u\n",*ret_event,*ret_node_id);

	return ERR_OK;
}

int tc_client_topic_create( unsigned int topic_id, unsigned int size, unsigned int period )
{
	DEBUG_MSG_TC_CLIENT("tc_client_topic_create() TOPIC ID %u ...\n",topic_id);

	NET_MSG	msg;

	if ( !init ){
		fprintf(stderr,"tc_client_topic_create() : MODULE IS NOT INITIALIZED\n");
		return ERR_C_NOT_INIT;
	}	

	//Validate parameters
	if ( !topic_id || !size || !period ){
		fprintf(stderr,"tc_client_topic_create() : INVALID PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Prepare request
	memset(&msg,0,sizeof(NET_MSG));

	msg.type = REQ_MSG;
	msg.op = REG_TOPIC;
	msg.node_ids[0] = tc_node_id;
	msg.n_nodes = 1;
	msg.topic_id = topic_id;
	msg.channel_size = size;
	msg.channel_period = period;
	
	//Get in requests queue
	tc_client_get_server_access();

	//Send request
	if ( tc_network_send_msg( &server_sock, &msg, NULL ) ){
		fprintf(stderr,"tc_client_topic_create() : ERROR SENDING REQUEST FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_SEND_REQUEST;
	}
	
	DEBUG_MSG_TC_CLIENT("tc_client_topic_create() : Waiting for topic id %u creation request response\n",topic_id);

	//Receive answer
	memset(&msg,0,sizeof(NET_MSG));

	if ( tc_network_get_msg( &server_sock, C_REQUESTS_TIMEOUT, &msg, NULL ) ){
		fprintf(stderr,"tc_client_topic_create() : ERROR RECEIVING REQUEST ANSWER FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_GET_ANSWER;
	}

	//Leave requests queue
	tc_client_release_server_access();

	//Check if operation was successfull
	if( msg.type != ANS_MSG || msg.error || msg.node_ids[0] != tc_node_id ){
		fprintf(stderr,"tc_client_topic_create() : SERVER DENIED CREATION OF TOPIC ID %u\n",topic_id);
		return msg.error;
	}

	DEBUG_MSG_TC_CLIENT("tc_client_topic_create() Topic Id %u created\n",topic_id);

	return ERR_OK;		
}

int tc_client_topic_destroy( unsigned int topic_id )
{
	DEBUG_MSG_TC_CLIENT("tc_client_topic_destroy() TOPIC ID %u ...\n",topic_id);

	NET_MSG msg;

	if ( !init ){
		fprintf(stderr,"tc_client_topic_destroy() : MODULE IS NOT INITIALIZED\n");
		return ERR_C_NOT_INIT;
	}	

	//Validate parameters
	if ( !topic_id ){
		fprintf(stderr,"tc_client_topic_destroy() : INVALID PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Prepare request
	memset(&msg,0,sizeof(NET_MSG));

	msg.type = REQ_MSG;
	msg.op = DEL_TOPIC;
	msg.node_ids[0] = tc_node_id;
	msg.n_nodes = 1;
	msg.topic_id = topic_id;
	
	//Get in requests queue
	tc_client_get_server_access();

	//Send request
	if ( tc_network_send_msg( &server_sock, &msg, NULL ) ){
		fprintf(stderr,"tc_client_topic_destroy() : ERROR SENDING REQUEST FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_SEND_REQUEST;
	}
	
	DEBUG_MSG_TC_CLIENT("tc_client_topic_destroy() : Waiting for topic id %u removal request response\n",topic_id);

	//Receive answer
	memset(&msg,0,sizeof(NET_MSG));

	if ( tc_network_get_msg( &server_sock, C_REQUESTS_TIMEOUT, &msg, NULL ) ){
		fprintf(stderr,"tc_client_topic_destroy() : ERROR RECEIVING REQUEST ANSWER FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_GET_ANSWER;
	}

	//Leave requests queue
	tc_client_release_server_access();

	//Check if operation was successfull
	if( msg.type != ANS_MSG || msg.error || msg.node_ids[0] != tc_node_id ){
		fprintf(stderr,"tc_client_topic_destroy() : SERVER DENIED DESTRUCTION OF TOPIC ID %u\n",topic_id);
		return msg.error;
	}

	DEBUG_MSG_TC_CLIENT("tc_client_topic_destroy() Topic Id %u destroyed\n",topic_id);

	return ERR_OK;		
}

int tc_client_topic_get_prop( unsigned int topic_id , unsigned int *ret_size, unsigned int *ret_period )
{
	DEBUG_MSG_TC_CLIENT(" tc_client_topic_get_prop() TOPIC ID %u ...\n",topic_id);

	NET_MSG msg;

	if ( !init ){
		fprintf(stderr," tc_client_topic_get_prop() : MODULE IS NOT INITIALIZED\n");
		return ERR_C_NOT_INIT;
	}	

	//Validate parameters
	if ( !topic_id ){
		fprintf(stderr,"tc_client_topic_get_prop() : INVALID PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Get topic channel properties
	//Prepare request
	memset(&msg,0,sizeof(NET_MSG));

	msg.type = REQ_MSG;
	msg.op = GET_TOPIC_PROP;
	msg.node_ids[0] = tc_node_id;
	msg.n_nodes = 1;
	msg.topic_id  = topic_id;

	//Get in requests queue
	tc_client_get_server_access();

	//Send request
	if ( tc_network_send_msg( &server_sock, &msg, NULL ) ){
		fprintf(stderr,"tc_client_topic_get_prop() : ERROR SENDING REQUEST FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_SEND_REQUEST;
	}
	
	DEBUG_MSG_TC_CLIENT(" tc_client_topic_get_prop() : Waiting for topic id %u request response\n",topic_id);

	//Get answer
	memset(&msg,0,sizeof(NET_MSG));

	if ( tc_network_get_msg( &server_sock, C_REQUESTS_TIMEOUT, &msg, NULL ) ){
		fprintf(stderr,"tc_client_topic_get_prop() : ERROR RECEIVING REQUEST ANSWER FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_GET_ANSWER;
	}

	//Check if request was successfull
	if( msg.type != ANS_MSG || msg.error || msg.node_ids[0] != tc_node_id ){
		fprintf(stderr," tc_client_topic_get_prop() : SERVER DECLINED REQUEST FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return msg.error;
	}

	if ( ret_size ) *ret_size = msg.channel_size;
	if ( ret_period ) *ret_period = msg.channel_period;
			
	tc_client_release_server_access();

	DEBUG_MSG_TC_CLIENT(" tc_client_topic_get_prop() Got topic id %u details\n",topic_id);

	return ERR_OK;
}

int tc_client_topic_set_prop( unsigned int topic_id , unsigned int new_size, unsigned int new_period )
{
	DEBUG_MSG_TC_CLIENT(" tc_client_topic_set_prop() TOPIC ID %u ...\n",topic_id);

	NET_MSG msg;

	if ( !init ){
		fprintf(stderr," tc_client_topic_set_prop() : MODULE IS NOT INITIALIZED\n");
		return ERR_C_NOT_INIT;
	}	

	//Validate parameters
	if ( !topic_id || !new_size || !new_period ){
		fprintf(stderr,"tc_client_topic_set_prop() : INVALID PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Prepare request
	memset(&msg,0,sizeof(NET_MSG));

	msg.type = REQ_MSG;
	msg.op = SET_TOPIC_PROP;
	msg.node_ids[0] = tc_node_id;
	msg.n_nodes = 1;
	msg.topic_id  = topic_id;
	msg.channel_size = new_size;
	msg.channel_period = new_period;

	//Get in requests queue
	tc_client_get_server_access();

	//Send request
	if ( tc_network_send_msg( &server_sock, &msg, NULL ) ){
		fprintf(stderr,"ttc_client_topic_set_prop() : ERROR SENDING REQUEST FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_SEND_REQUEST;
	}
	
	DEBUG_MSG_TC_CLIENT("tc_client_topic_set_prop() : Waiting for topic id %d request response\n",topic_id);

	//Get answer
	memset(&msg,0,sizeof(NET_MSG));

	if ( tc_network_get_msg( &server_sock, C_REQUESTS_TIMEOUT, &msg, NULL ) ){
		fprintf(stderr,"tc_client_topic_set_prop() : ERROR RECEIVING REQUEST ANSWER FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_GET_ANSWER;
	}

	//Check if request was successfull
	if( msg.type != ANS_MSG || msg.error || msg.node_ids[0] != tc_node_id ){
		fprintf(stderr," tc_client_topic_set_prop() : SERVER DECLINED REQUEST FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return msg.error;
	}
			
	tc_client_release_server_access();

	DEBUG_MSG_TC_CLIENT("tc_client_topic_set_prop() Set topic id %u with the new properties\n",topic_id);

	return ERR_OK;
}

int tc_client_register_tx( unsigned int topic_id )
{
	DEBUG_MSG_TC_CLIENT("tc_client_register_tx() TOPIC ID %u ...\n",topic_id);

	NET_MSG msg;
	TOPIC_C_ENTRY *topic = NULL;

	if ( !init ){
		fprintf(stderr,"tc_client_register_tx() : MODULE IS NOT INITIALIZED\n");
		return ERR_C_NOT_INIT;
	}	

	//Validate parameters
	if ( !topic_id ){
		fprintf(stderr,"tc_client_register_tx() : INVALID PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Get in requests queue
	tc_client_get_server_access();

	//Lock topic database
	tc_client_db_lock();

	//Check if theres a local entry for this topic and if this node is already producer
	if ( (topic = tc_client_db_topic_search(topic_id)) && topic->is_producer ){
		DEBUG_MSG_TC_CLIENT("tc_client_register_tx() : Node is already a producer of topic id %d\n",topic_id);
		tc_client_db_unlock();
		tc_client_release_server_access();
		return ERR_OK;
	}

	//Prepare request
	memset(&msg,0,sizeof(NET_MSG));

	msg.type = REQ_MSG;
	msg.op = REG_PROD;
	msg.node_ids[0] = tc_node_id;
	msg.n_nodes = 1;
	msg.topic_id  = topic_id;

	//Unlock topic database
	tc_client_db_unlock();

	//Send request
	if ( tc_network_send_msg( &server_sock, &msg, NULL ) ){
		fprintf(stderr,"tc_client_register_tx() : ERROR SENDING REQUEST FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_SEND_REQUEST;
	}
	
	DEBUG_MSG_TC_CLIENT("tc_client_register_tx() : Waiting for registration as producer of topic id %u request response\n",topic_id);

	//Get answer
	memset(&msg,0,sizeof(NET_MSG));

	if ( tc_network_get_msg( &server_sock, C_REQUESTS_TIMEOUT, &msg, NULL ) ){
		fprintf(stderr,"tc_client_register_tx() : ERROR RECEIVING REQUEST ANSWER FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_GET_ANSWER;
	}

	//Check if registration was successfull
	if( msg.type != ANS_MSG || msg.error || msg.node_ids[0] != tc_node_id ){
		fprintf(stderr,"tc_client_register_tx() : SERVER DENIED REGISTRATION AS PRODUCER OF TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return msg.error;
	}

	//Set host address
	NET_ADDR host;
	memset(&host,0,sizeof(NET_ADDR));
	strcpy(host.name_ip,nic_ip);
	host.port = msg.topic_addr.port;

	//Set peer address
	NET_ADDR peer;
	memset(&peer,0,sizeof(NET_ADDR));
	strcpy(peer.name_ip, msg.topic_addr.name_ip);
	peer.port = msg.topic_addr.port;
 
	printf("tc_client_register_tx() : Topic Id %u going to join group %s:%u\n",topic_id,peer.name_ip,peer.port);

	//Lock topic database
	tc_client_db_lock();

	if ( !(topic = tc_client_db_topic_search(topic_id)) ){
		//No entry for this topic -> create new one
		if ( !(topic = tc_client_db_topic_create(topic_id)) ){
			fprintf(stderr,"tc_client_register_tx() : ERROR CREATING ENTRY FOR TOPIC ID %u\n",topic_id);
			tc_client_db_unlock();
			tc_client_release_server_access();
			return ERR_TOPIC_LOCAL_CREATE;
		}
	}

	if ( topic->topic_sock.fd <= 0 ){
		//No socket exists for this topic -> create one
		if ( sock_open(&topic->topic_sock, REMOTE_UDP_GROUP ) ){
			fprintf(stderr,"tc_client_register_tx() : ERROR CREATING SOCKET FOR TOPIC ID %u\n",topic_id);
			tc_client_db_topic_delete( topic );
			tc_client_db_unlock();
			tc_client_release_server_access();
			return ERR_SOCK_CREATE;
		}
		//Bind socket to group port
		if ( sock_bind(&topic->topic_sock, &host ) ){
			fprintf(stderr,"tc_client_register_tx() : ERROR BINDING SOCKET TO GROUP OF TOPIC ID %u\n",topic_id);
			tc_client_db_topic_delete( topic );
			tc_client_db_unlock();
			tc_client_release_server_access();
			return ERR_SOCK_BIND_PEER;
		}
			
	}

	//Join topic as producer
	if ( !topic->is_producer ){//FailSafe
		if ( sock_connect_group_tx( &topic->topic_sock, &peer ) ){
			fprintf(stderr,"tc_client_register_tx() : ERROR REGISTERING AS TOPIC ID %u PRODUCER\n",topic_id);
			tc_client_db_topic_delete( topic );
			tc_client_db_unlock();
			tc_client_release_server_access();
			return ERR_TOPIC_JOIN_TX;
		}
	}

	//Update topic entry
	topic->topic_id = topic_id;
	topic->topic_addr = msg.topic_addr;
	topic->channel_size = msg.channel_size;
	topic->channel_period = msg.channel_period;
	topic->is_producer = 1;	
	
	//Unlock topic database
	tc_client_db_unlock();

	//Leave requests queue
	tc_client_release_server_access();

	DEBUG_MSG_TC_CLIENT("tc_client_register_tx() Registered as producer of topic ID %u\n",topic_id);

	return ERR_OK;
}

int tc_client_unregister_tx( unsigned int topic_id )
{
	DEBUG_MSG_TC_CLIENT("tc_client_unregister_tx() TOPIC ID %u ...\n",topic_id);

	NET_MSG msg;
	TOPIC_C_ENTRY *topic = NULL;
	
	if ( !init ){
		fprintf(stderr,"tc_client_unregister_tx() : MODULE IS NOT INITIALIZED\n");
		return ERR_C_NOT_INIT;
	}	

	//Validate parameters
	if ( !topic_id ){
		fprintf(stderr,"tc_client_unregister_tx(): INVALID PARAMETERS\n");
		return ERR_INVALID_PARAM;	
	}

	//Get in requests queue
	tc_client_get_server_access();

	//Lock topic database
	tc_client_db_lock();

	//Check if theres a local entry for this topic and if this node is registered as producer
	if ( !(topic = tc_client_db_topic_search(topic_id) ) || !topic->is_producer ){
		DEBUG_MSG_TC_CLIENT("tc_client_unregister_tx() : Node is not registered as producer of topic id %u\n",topic_id);
		tc_client_db_unlock();
		tc_client_release_server_access();
		return ERR_OK;
	}

	//Prepare request
	memset(&msg,0,sizeof(NET_MSG));

	msg.type = REQ_MSG;
	msg.op = UNREG_PROD;
	msg.node_ids[0] = tc_node_id;
	msg.n_nodes = 1;
	msg.topic_id  = topic_id;

	//Unlock topic database
	tc_client_db_unlock();

	//Send request
	if ( tc_network_send_msg( &server_sock, &msg, NULL ) ){
		fprintf(stderr,"tc_client_unregister_tx() : ERROR SENDING REQUEST FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_SEND_REQUEST;
	}
	
	DEBUG_MSG_TC_CLIENT("tc_client_unregister_tx() : Waiting for topic id %d request response\n",topic_id);

	//Get answer
	memset(&msg,0,sizeof(NET_MSG));

	if ( tc_network_get_msg( &server_sock, C_REQUESTS_TIMEOUT, &msg, NULL ) ){
		fprintf(stderr,"tc_client_unregister_tx() : ERROR RECEIVING REQUEST ANSWER FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_GET_ANSWER;
	}

	//Check if unregistration request was accepted
	if( msg.type != ANS_MSG || msg.error || msg.node_ids[0] != tc_node_id ){
		fprintf(stderr,"tc_client_unregister_tx() : SERVER DENIED UNREGISTRATION AS PRODUCER OF TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return msg.error;
	}

	//Lock topic database
	tc_client_db_lock();

	//Check for db changes
	//Check if theres a local entry for this topic and if this node is registered as producer
	if ( !(topic = tc_client_db_topic_search(topic_id) ) || !topic->is_producer ){
		DEBUG_MSG_TC_CLIENT("tc_client_unregister_tx() : Node is not registered as producer of topic id %u\n",topic_id);
		tc_client_db_unlock();
		tc_client_release_server_access();
		return ERR_NODE_NOT_REG_TX;
	}
	
	//NOTE : We can't unregister from a group without closing the socket

	//Unbind as producer and signal topic as updating
	topic->is_tx_bound = 0;
	topic->is_updating = 1;
	sock_close( &topic->topic_sock );
	topic->topic_sock.fd = 0;
	topic->is_producer = 0;

	//If we are registered as consumer we need to create a new socket and rejoin group as consumer only
	if ( topic->is_consumer ){

		//Send message to unblock socket -- Failsafe
		sock_send( &topic->unblock_rx_sock, &topic->unblock_rx_sock.host, "0", 5 );
		usleep(1000);

		if ( sock_open(&topic->topic_sock, REMOTE_UDP_GROUP ) ){
			fprintf(stderr,"tc_client_unregister_tx() : ERROR CREATING SOCKET FOR TOPIC ID %u (REJOIN AS CONSUMER)\n",topic->topic_id);
			//Delete topic to trigger an error to the application so it will reset the registration process
			tc_client_db_topic_delete( topic );
			tc_client_db_unlock();
			tc_client_release_server_access();
			topic->is_updating = 0;
			return ERR_SOCK_CREATE;
		}
	
		//Set host address
		NET_ADDR host;
		memset(&host,0,sizeof(NET_ADDR));
		strcpy(host.name_ip,nic_ip);
		host.port = topic->topic_addr.port;

		//Set peer address
		NET_ADDR peer;
		memset(&peer,0,sizeof(NET_ADDR));
		strcpy(peer.name_ip, topic->topic_addr.name_ip);
		peer.port = topic->topic_addr.port;
	 
		//Bind socket to group port
		if ( sock_bind(&topic->topic_sock, &host ) ){
			fprintf(stderr,"tc_client_unregister_tx() : ERROR BINDING SOCKET TO GROUP OF TOPIC ID %u\n",topic_id);
			//Delete topic entry for app to reset registration (as producer and consumer if required)
			tc_client_db_topic_delete( topic );
			tc_client_db_unlock();
			tc_client_release_server_access();
			topic->is_updating = 0;
			return ERR_SOCK_BIND_PEER;
		}

		//Join topic as consumer
		if ( sock_connect_group_rx( &topic->topic_sock, &peer ) ){
			fprintf(stderr,"tc_client_unregister_tx() : ERROR JOINING TOPIC ID %u AS CONSUMER\n",topic_id);
			tc_client_db_topic_delete( topic );
			tc_client_db_unlock();
			tc_client_release_server_access();
			topic->is_updating = 0;
			return ERR_TOPIC_JOIN_RX;
		}
	}
	
	//Update done
	topic->is_updating = 0;

	//Unlock topic database
	tc_client_db_unlock();

	//Leave requests queue
	tc_client_release_server_access();

	DEBUG_MSG_TC_CLIENT("tc_client_unregister_tx() Unregistered as producer of topic id %d\n",topic_id);

	return ERR_OK;
}

int tc_client_register_rx( unsigned int topic_id )
{
	DEBUG_MSG_TC_CLIENT("tc_client_register_rx() TOPIC ID %u ...\n",topic_id);

	NET_MSG msg;
	TOPIC_C_ENTRY *topic = NULL;

	if ( !init ){
		fprintf(stderr,"tc_client_register_rx() : MODULE IS NOT INITIALIZED\n");
		return ERR_C_NOT_INIT;
	}	

	//Validate parameters
	if ( !topic_id ){
		fprintf(stderr,"tc_client_register_rx() : INVALID PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Get in requests queue
	tc_client_get_server_access();

	//Lock topic database
	tc_client_db_lock();

	//Check if this node is already registered as consumer of this topic
	topic = tc_client_db_topic_search(topic_id);

	if ( topic && topic->is_consumer ){
		DEBUG_MSG_TC_CLIENT("tc_client_register_rx() : Node is already a consumer of topic id %u\n",topic_id);
		tc_client_db_unlock();
		tc_client_release_server_access();
		return ERR_OK;
	}
	
	//Prepare request
	memset(&msg,0,sizeof(NET_MSG));

	msg.type = REQ_MSG;
	msg.op = REG_CONS;
	msg.node_ids[0] = tc_node_id;
	msg.n_nodes = 1;
	msg.topic_id  = topic_id;

	//Unlock topic database
	tc_client_db_unlock();

	//Send request
	if ( tc_network_send_msg( &server_sock, &msg, NULL ) ){
		fprintf(stderr,"tc_client_register_rx() : ERROR SENDING REQUEST FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_SEND_REQUEST;
	}

	DEBUG_MSG_TC_CLIENT("tc_client_register_rx() : Waiting for topic id %d request response\n",topic_id);

	//Get answer
	memset(&msg,0,sizeof(NET_MSG));

	if ( tc_network_get_msg( &server_sock, C_REQUESTS_TIMEOUT, &msg, NULL ) ){
		fprintf(stderr,"tc_client_register_rx() : ERROR RECEIVING REQUEST ANSWER FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_GET_ANSWER;
	}
	
	//Check if registration was successfull
	if( msg.type != ANS_MSG || msg.error || msg.node_ids[0] != tc_node_id ){
		fprintf(stderr,"tc_client_register_rx() : SERVER DENIED REGISTRATION AS CONSUMER OF TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return msg.error;
	}

	//Set host address
	NET_ADDR host;
	memset(&host,0,sizeof(NET_ADDR));
	strcpy(host.name_ip,nic_ip);
	host.port = msg.topic_addr.port;

	//Set peer address
	NET_ADDR peer;
	memset(&peer,0,sizeof(NET_ADDR));
	strcpy(peer.name_ip, msg.topic_addr.name_ip);
	peer.port = msg.topic_addr.port;
 
	printf("tc_client_register_rx() : Topic Id %u going to join group %s:%u\n",topic_id,peer.name_ip,peer.port);

	//Lock topic database
	tc_client_db_lock();

	if ( !(topic = tc_client_db_topic_search(topic_id)) ){
		//No entry for this topic -> create new one
		if ( !(topic = tc_client_db_topic_create(topic_id)) ){
			fprintf(stderr,"tc_client_register_rx() : ERROR CREATING ENTRY FOR TOPIC ID %u\n",topic_id);
			tc_client_db_unlock();
			tc_client_release_server_access();
			return ERR_TOPIC_LOCAL_CREATE;
		}
	}

	if ( topic->topic_sock.fd <= 0 ){
		printf("tc_client_register_rx() : CREATING SOCKET FOR TOPIC ID %u\n",topic_id);
		//No socket exists for this topic -> create one
		if ( sock_open(&topic->topic_sock, REMOTE_UDP_GROUP ) ){
			fprintf(stderr,"tc_client_register_rx() : ERROR CREATING SOCKET FOR TOPIC ID %u\n",topic_id);
			tc_client_db_topic_delete( topic );
			tc_client_db_unlock();
			tc_client_release_server_access();
			return ERR_SOCK_CREATE;
		}

		//Bind socket to group port
		if ( sock_bind(&topic->topic_sock, &host ) ){
			fprintf(stderr,"tc_client_register_rx() : ERROR BINDING SOCKET TO GROUP OF TOPIC ID %u\n",topic_id);
			//Delete topic entry for app to reset registration (as producer and consumer if required)
			tc_client_db_topic_delete( topic );
			tc_client_db_unlock();
			tc_client_release_server_access();
			return ERR_SOCK_BIND_PEER;
		}
	}

	if ( topic->unblock_rx_sock.fd <= 0 ){
		//No unblock socket exists for this topic -> create one
		if ( sock_open(&topic->unblock_rx_sock, LOCAL) ){
			fprintf(stderr,"tc_client_register_rx() : ERROR CREATING UNBLOCK SOCKET FOR TOPIC ID %u\n",topic_id);
			tc_client_db_topic_delete( topic );
			tc_client_db_unlock();
			tc_client_release_server_access();
			return ERR_SOCK_CREATE;
		}

		//Set local name for unblock local socket
		sprintf(topic->unblock_rx_sock.host.name_ip,"tc_unblock_%u",topic_id);
		if ( sock_bind(&topic->unblock_rx_sock, &topic->unblock_rx_sock.host ) ){
			fprintf(stderr,"tc_client_register_rx() : ERROR BINDING UNBLOCK RX SOCKET OF TOPIC ID %u\n",topic_id);
			//Delete topic entry for app to reset registration (as producer and consumer if required)
			tc_client_db_topic_delete( topic );
			tc_client_db_unlock();
			tc_client_release_server_access();
			return ERR_SOCK_BIND_HOST;
		} 
	}

	//Join topic as consumer
	if ( !topic->is_consumer ){//FailSafe
		if ( sock_connect_group_rx( &topic->topic_sock, &peer ) ){
			fprintf(stderr,"tc_client_register_rx() : ERROR REGISTERING AS TOPIC ID %u PRODUCER\n",topic_id);
			tc_client_db_topic_delete( topic );
			tc_client_db_unlock();
			tc_client_release_server_access();
			return ERR_TOPIC_JOIN_RX;
		}
	}

	//Update topic entry
	topic->topic_id = topic_id;
	topic->topic_addr = msg.topic_addr;
	topic->channel_size = msg.channel_size;
	topic->channel_period = msg.channel_period;
	topic->is_consumer = 1;

	//Unlock topic database
	tc_client_db_unlock();

	//Leave requests queue
	tc_client_release_server_access();

	DEBUG_MSG_TC_CLIENT("tc_client_register_rx() Registered as consumer of topic ID %u\n",topic_id);

	return ERR_OK;
}

int tc_client_unregister_rx( unsigned int topic_id )
{
	DEBUG_MSG_TC_CLIENT("tc_client_unregister_rx() TOPIC ID %u ...\n",topic_id);

	NET_MSG msg;
	TOPIC_C_ENTRY *topic = NULL;

	if ( !init ){
		fprintf(stderr,"tc_client_unregister_rx() : MODULE IS NOT RUNNING\n");
		return ERR_C_NOT_INIT;
	}	

	//Validate parameters
	if ( !topic_id ){
		fprintf(stderr,"tc_client_unregister_rx() : INVALID PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Get in requests queue
	tc_client_get_server_access();

	//Lock topic database
	tc_client_db_lock();

	//Check if theres a local entry for this topic and if this node is registered as consumer
	if ( !(topic = tc_client_db_topic_search(topic_id) ) || !topic->is_consumer ){
		DEBUG_MSG_TC_CLIENT("tc_client_unregister_rx() : Node is not registered as consumer of topic id %u\n",topic_id);
		tc_client_db_unlock();
		tc_client_release_server_access();
		return ERR_OK;
	}

	//Prepare request
	memset(&msg,0,sizeof(NET_MSG));

	msg.type = REQ_MSG;
	msg.op = UNREG_CONS;
	msg.node_ids[0] = tc_node_id;
	msg.n_nodes = 1;
	msg.topic_id  = topic_id;

	//Unlock topic database
	tc_client_db_unlock();

	//Send request
	if ( tc_network_send_msg( &server_sock, &msg, NULL ) ){
		fprintf(stderr,"tc_client_unregister_rx() : ERROR SENDING REQUEST FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_SEND_REQUEST;
	}
	
	DEBUG_MSG_TC_CLIENT("tc_client_unregister_rx() : Waiting for topic id %u request response\n",topic_id);

	//Get answer
	if ( tc_network_get_msg( &server_sock, C_REQUESTS_TIMEOUT, &msg, NULL ) ){
		fprintf(stderr,"tc_client_unregister_rx() : ERROR RECEIVING REQUEST ANSWER FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_GET_ANSWER;
	}

	//Check if unregistration request was accepted
	if( msg.type != ANS_MSG || msg.error || msg.node_ids[0] != tc_node_id ){
		fprintf(stderr,"tc_client_unregister_rx() : SERVER DENIED UNREGISTRATION AS CONSUMER OF TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return msg.error;
	}

	//Lock topic database
	tc_client_db_lock();

	//Check if theres a local entry for this topic and if this node is registered as consumer (in case topic was destroyed)
	if ( !(topic = tc_client_db_topic_search(topic_id) ) || !topic->is_consumer ){
		DEBUG_MSG_TC_CLIENT("tc_client_unregister_rx() : Node is not registered as consumer of topic id %u\n",topic_id);
		tc_client_db_unlock();
		tc_client_release_server_access();
		return ERR_NODE_NOT_REG_RX;
	}
	
	//NOTE : We can't unregister from a group without closing the socket
	//Unbind and signal topic as updating
	topic->is_rx_bound = 0;
	topic->is_updating = 1;

	//Send message to unblock socket
	sock_send( &topic->unblock_rx_sock, &topic->unblock_rx_sock.host, "0", 5 );
	usleep(10000);

	sock_close( &topic->topic_sock );
	topic->topic_sock.fd = 0;
	topic->is_consumer = 0;
	topic->is_rx_bound = 0;

	//If we are registered as producer we need to create a new socket and rejoin group as producer only
	if ( topic->is_producer ){
		if ( sock_open(&topic->topic_sock, REMOTE_UDP_GROUP ) ){
			fprintf(stderr,"tc_client_unregister_rx() : ERROR CREATING SOCKET FOR TOPIC ID %u (REJOIN AS PRODUCER)\n",topic->topic_id);
			//Delete topic to trigger an error to the application so it will reset the join process
			tc_client_db_topic_delete(topic);
			tc_client_db_unlock();
			tc_client_release_server_access();
			topic->is_updating = 0;
			return ERR_SOCK_CREATE;
		}
		
		//Set host address
		NET_ADDR host;
		memset(&host,0,sizeof(NET_ADDR));
		strcpy(host.name_ip,nic_ip);

		//Set peer address
		NET_ADDR peer;
		memset(&peer,0,sizeof(NET_ADDR));
		strcpy(peer.name_ip, topic->topic_addr.name_ip);
		peer.port = topic->topic_addr.port;
	 
		//Bind socket to group port
		if ( sock_bind(&topic->topic_sock, &host ) ){
			fprintf(stderr,"tc_client_unregister_rx() : ERROR BINDING SOCKET TO GROUP OF TOPIC ID %u\n",topic_id);
			//Delete topic entry for app to reset registration (as producer and consumer if required)
			tc_client_db_topic_delete( topic );
			tc_client_db_unlock();
			tc_client_release_server_access();
			topic->is_updating = 0;
			return ERR_SOCK_BIND_PEER;
		}

		//Join topic as producer
		if ( sock_connect_group_tx( &topic->topic_sock, &peer ) ){
			fprintf(stderr,"tc_client_unregister_rx() : ERROR JOINING TOPIC ID %u AS PRODUCER\n",topic_id);
			tc_client_db_topic_delete(topic);
			tc_client_db_unlock();
			tc_client_release_server_access();
			topic->is_updating = 0;
			return ERR_TOPIC_JOIN_TX;
		}
	}
	
	//Update done
	topic->is_updating = 0;

	//Unlock topic database
	tc_client_db_unlock();

	//Leave requests queue
	tc_client_release_server_access();

	DEBUG_MSG_TC_CLIENT("tc_client_unregister_rx() Unregistered as consumer of topic ID %u\n",topic_id);

	return ERR_OK;
}

int tc_client_bind_tx( unsigned int topic_id, unsigned int timeout )
{
	DEBUG_MSG_TC_CLIENT("tc_client_bind_tx() TOPIC ID %u ...\n",topic_id);

	NET_MSG msg;
	int tries = 0;
	TOPIC_C_ENTRY *topic = NULL;

	if ( !init ){
		fprintf(stderr,"tc_client_bind_tx() : MODULE IS NOT INITIALIZED\n");
		return ERR_C_NOT_INIT;
	}	

	//Validate parameters
	if ( !topic_id ){
		fprintf(stderr,"tc_client_bind_tx() : INVALID PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Get in requests queue
	tc_client_get_server_access();

	//Lock topic database
	tc_client_db_lock();

	//Check if node is registered as producer of this topic 
	if ( !(topic = tc_client_db_topic_search(topic_id)) || !topic->is_producer ){
		fprintf(stderr,"tc_client_bind_tx() : NOT REGISTERED AS PRODUCER OF TOPIC ID %u\n",topic_id);
		tc_client_db_unlock();
		tc_client_release_server_access();
		return ERR_NODE_NOT_REG_TX;
	}

	//Check if its already bound
	if ( topic->is_tx_bound ){
		DEBUG_MSG_TC_CLIENT("tc_client_bind_tx() : Already bound to topic id %u as producer\n",topic_id);
		tc_client_db_unlock();
		tc_client_release_server_access();
		return ERR_OK;	
	}

	//Prepare request
	memset(&msg,0,sizeof(NET_MSG));

	msg.type = REQ_MSG;
	msg.op = BIND_TX;
	msg.node_ids[0] = tc_node_id;
	msg.n_nodes = 1;
	msg.topic_id  = topic_id;

	//Unlock topic database
	tc_client_db_unlock();

	//Send request
	if ( tc_network_send_msg( &server_sock, &msg, NULL ) ){
		fprintf(stderr,"tc_client_bind_tx() : ERROR SENDING REQUEST FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_SEND_REQUEST;
	}
	
	DEBUG_MSG_TC_CLIENT("tc_client_bind_tx() : Waiting for topic id %u bind request response\n",topic_id);

	//Get answer
	memset(&msg,0,sizeof(NET_MSG));

	if ( tc_network_get_msg( &server_sock, C_REQUESTS_TIMEOUT, &msg, NULL ) ){
		fprintf(stderr,"tc_client_bind_tx() : ERROR RECEIVING REQUEST ANSWER FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_GET_ANSWER;
	}

	//Check if bind request was successfull
	if( msg.type != ANS_MSG || msg.error || msg.node_ids[0] != tc_node_id ){
		fprintf(stderr,"tc_client_bind_tx() : SERVER DENIED BIND PROCEDURE REQUEST FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return msg.error;
	}

	//Request was sent and accepted. It will wait for a notification from server to finalize the bind
	//However server will only reply when a least 1 consumer and 1 producer for the same topic are registered and bound (or in bind process)
	//So we could be waiting here for a long time and block other threads from doing requests to server
	//The bind request answer will be sent to the management module to avoid this issue and we will wait on the bind flag here

	//Leave requests queue
	tc_client_release_server_access();

	//Wait for bind procedure to be completed
	//Get entry for topic
	if ( !(topic = tc_client_db_topic_search(topic_id)) ){
		fprintf(stderr,"tc_client_bind_tx(): TOPIC ID %u HAS BEEN DESTROYED WHILE WAITING FOR BIND\n",topic_id);
		return ERR_TOPIC_NOT_REG;
	}

	if ( timeout > 0 ){

		//Bind with timeout
		for ( tries = 0; tries < timeout && !topic->is_tx_bound; tries++ ){

			usleep(1000);

			if ( !(topic = tc_client_db_topic_search(topic_id)) ){
				fprintf(stderr,"tc_client_bind_tx(): TOPIC ID %u HAS BEEN DESTROYED WHILE WAITING FOR BIND\n",topic_id);
				return ERR_TOPIC_NOT_REG;
			}
		}
	
	}else{
		//Bind without timeout
		while ( !topic->is_tx_bound ){

			usleep(1000);
			//Get entry for topic
			if ( !(topic = tc_client_db_topic_search(topic_id)) ){
				fprintf(stderr,"tc_client_bind_tx(): TOPIC ID %u HAS BEEN DESTROYED WHILE WAITING FOR BIND\n",topic_id);
				return ERR_TOPIC_NOT_REG;
			}
		}
	}

	if ( !topic->is_tx_bound ){
		fprintf(stderr,"tc_client_bind_tx() : TIMED-OUT WAITING TO BIND TO TOPIC ID %u\n",topic_id);
		return ERR_BIND_TX_TIMEDOUT;
	}

	DEBUG_MSG_TC_CLIENT("tc_client_bind_tx() : Bound to topic ID %u as producer\n",topic_id);

	return ERR_OK;
}

int tc_client_unbind_tx( unsigned int topic_id )
{
	DEBUG_MSG_TC_CLIENT("tc_client_unbind_tx() TOPIC ID %u ...\n",topic_id);

	NET_MSG msg;
	TOPIC_C_ENTRY *topic = NULL;
	int tries = UNBIND_TIMEOUT;

	if ( !init ){
		fprintf(stderr,"tc_client_unbind_tx() : MODULE IS NOT INITIALIZED\n");
		return ERR_C_NOT_INIT;
	}	

	//Validate parameters
	if ( !topic_id ){
		fprintf(stderr,"tc_client_unbind_tx() : INVALID PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Get in requests queue
	tc_client_get_server_access();

	//Lock topic database
	tc_client_db_lock();

	//Check if its registered as producer of this topic 
	if ( !(topic = tc_client_db_topic_search( topic_id )) || !topic->is_producer ){
		fprintf(stderr,"tc_client_unbind_tx() : NOT REGISTERED AS PRODUCER OF TOPIC ID %u\n",topic_id);
		tc_client_db_unlock();
		tc_client_release_server_access();
		return ERR_NODE_NOT_REG_TX;
	}

	//Check if node is bound to topic as producer
	if ( !topic->is_tx_bound ){
		DEBUG_MSG_TC_CLIENT("tc_client_unbind_tx() : Node not bound to topic ID %u as producer\n",topic_id);
		tc_client_db_unlock();
		tc_client_release_server_access();
		return ERR_OK;	
	}

	//Prepare request
	memset(&msg,0,sizeof(NET_MSG));

	msg.type = REQ_MSG;
	msg.op = UNBIND_TX;
	msg.node_ids[0] = tc_node_id;
	msg.n_nodes = 1;
	msg.topic_id  = topic_id;

	//Unlock topic database
	tc_client_db_unlock();

	//Send request
	if ( tc_network_send_msg( &server_sock, &msg, NULL ) ){
		fprintf(stderr,"tc_client_unbind_tx() : ERROR SENDING REQUEST FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_SEND_REQUEST;
	}
	
	DEBUG_MSG_TC_CLIENT("tc_client_unbind_tx() : Waiting for topic id %u request response\n",topic_id);

	//Get answer
	memset(&msg,0,sizeof(NET_MSG));

	if ( tc_network_get_msg( &server_sock, C_REQUESTS_TIMEOUT, &msg, NULL ) ){
		fprintf(stderr,"tc_client_unbind_tx() : ERROR RECEIVING REQUEST ANSWER FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_GET_ANSWER;
	}

	//Check if unbind was successfull
	if( msg.type != ANS_MSG || msg.error || msg.node_ids[0] != tc_node_id ){
		fprintf(stderr,"tc_client_unbind_tx() : SERVER DENIED UNBIND AS PRODUCER FROM TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return msg.error;
	}

	//Lock topic database
	tc_client_db_lock();

	//Check if its registered as producer of this topic 
	if ( !(topic = tc_client_db_topic_search( topic_id )) || !topic->is_producer ){
		fprintf(stderr,"tc_client_unbind_tx() : NOT REGISTERED AS PRODUCER OF TOPIC ID %u\n",topic_id);
		tc_client_db_unlock();
		tc_client_release_server_access();
		return ERR_NODE_NOT_REG_TX;
	}

	//Unlock DB (required for management module)
	tc_client_db_unlock();

	//Wait for unbind -- Management module will receive a request from the server
	while( topic->is_tx_bound && tries-- )
		usleep(1000);

	if ( tries <= 0 ){
		fprintf(stderr,"tc_client_unbind_tx() : TIMEDOUT WHILE WAITING FOR UNBIND ON TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_UNBIND_TX_TIMEDOUT;
	}

	tc_client_release_server_access();

	DEBUG_MSG_TC_CLIENT("tc_client_unbind_tx() Unbound as a producer from topic id %u\n",topic_id);

	return ERR_OK;
}

int tc_client_bind_rx( unsigned int topic_id, unsigned int timeout )
{
	DEBUG_MSG_TC_CLIENT("tc_client_bind_rx() TOPIC ID %u ...\n",topic_id);

	NET_MSG msg;
	int tries = 0;
	TOPIC_C_ENTRY *topic = NULL;

	if ( !init ){
		fprintf(stderr,"tc_client_bind_rx() : MODULE IS NOT INITIALIZED\n");
		return ERR_C_NOT_INIT;
	}	

	//Validate parameters
	if ( !topic_id ){
		fprintf(stderr,"tc_client_bind_rx() : INVALID PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Get in requests queue
	tc_client_get_server_access();

	//Lock topic database
	tc_client_db_lock();

	//Check if node is registered as consumer of this topic 
	if ( !(topic = tc_client_db_topic_search(topic_id)) || !topic->is_consumer ){
		fprintf(stderr,"tc_client_bind_rx() : NOT REGISTERED AS CONSUMER OF TOPIC ID %u\n",topic_id);
		tc_client_db_unlock();
		tc_client_release_server_access();
		return ERR_NODE_NOT_REG_RX;
	}

	//Check if its already bound
	if ( topic->is_rx_bound ){
		DEBUG_MSG_TC_CLIENT("tc_client_bind_rx() : Already bound to topic id %u as consumer\n",topic_id);
		tc_client_db_unlock();
		tc_client_release_server_access();
		return ERR_OK;	
	}

	//Prepare request
	memset(&msg,0,sizeof(NET_MSG));

	msg.type = REQ_MSG;
	msg.op = BIND_RX;
	msg.node_ids[0] = tc_node_id;
	msg.n_nodes = 1;
	msg.topic_id  = topic_id;

	//Unlock topic database
	tc_client_db_unlock();

	//Send request
	if ( tc_network_send_msg( &server_sock, &msg, NULL ) ){
		fprintf(stderr,"tc_client_bind_rx() : ERROR SENDING REQUEST FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_SEND_REQUEST;
	}
	
	DEBUG_MSG_TC_CLIENT("tc_client_bind_rx() : Waiting for topic id %u bind request response\n",topic_id);

	//Get answer
	memset(&msg,0,sizeof(NET_MSG));

	if ( tc_network_get_msg( &server_sock, C_REQUESTS_TIMEOUT, &msg, NULL ) ){
		fprintf(stderr,"tc_client_bind_rx() : ERROR RECEIVING REQUEST ANSWER FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_GET_ANSWER;
	}

	//Check if bind request was successfull
	if( msg.type != ANS_MSG || msg.error || msg.node_ids[0] != tc_node_id ){
		fprintf(stderr,"tc_client_bind_rx() : SERVER DENIED BIND PROCEDURE REQUEST FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return msg.error;
	}

	//Request was sent and accepted. It will wait for a notification from server to finalize the bind
	//However server will only reply when a least 1 consumer and 1 producer for the same topic are registered and bound (or in bind process)
	//So we could be waiting here for a long time and block other threads from doing requests to server
	//The bind request answer will be sent to the management module to avoid this issue and we will wait on the bind flag

	//Leave requests queue
	tc_client_release_server_access();

	//Wait for bind procedure to be completed
	//Get entry for topic
	if ( !(topic = tc_client_db_topic_search(topic_id)) ){
		fprintf(stderr,"tc_client_bind_rx(): TOPIC ID %u HAS BEEN DESTROYED WHILE WAITING FOR BIND\n",topic_id);
		return ERR_TOPIC_NOT_REG;
	}

	if ( timeout > 0 ){

		//Bind with timeout
		for ( tries = 0; tries < timeout && !topic->is_rx_bound; tries++ ){

			usleep(1000);

			if ( !(topic = tc_client_db_topic_search(topic_id)) ){
				fprintf(stderr,"tc_client_bind_rx(): TOPIC ID %u HAS BEEN DESTROYED WHILE WAITING FOR BIND\n",topic_id);
				return ERR_TOPIC_NOT_REG;
			}
		}
	
	}else{
		//Bind without timeout
		while ( !topic->is_rx_bound ){

			usleep(1000);

			if ( !(topic = tc_client_db_topic_search(topic_id)) ){
				fprintf(stderr,"tc_client_bind_rx(): TOPIC ID %u HAS BEEN DESTROYED WHILE WAITING FOR BIND\n",topic_id);
				return ERR_TOPIC_NOT_REG;
			}
		}
	}

	if ( !topic->is_rx_bound ){
		fprintf(stderr,"tc_client_bind_rx() : TIMED-OUT WAITING TO BIND TO TOPIC ID %u\n",topic_id);
		return ERR_BIND_RX_TIMEDOUT;
	}

	DEBUG_MSG_TC_CLIENT("tc_client_bind_rx() : Bound to topic ID %u as consumer\n",topic_id);

	return ERR_OK;
}

int tc_client_unbind_rx( unsigned int topic_id )
{
	DEBUG_MSG_TC_CLIENT("tc_client_unbind_rx() TOPIC ID %u ...\n",topic_id);

	NET_MSG msg;
	TOPIC_C_ENTRY *topic = NULL;
	int tries = UNBIND_TIMEOUT;

	if ( !init ){
		fprintf(stderr,"tc_client_unbind_rx() : MODULE IS NOT INITIALIZED\n");
		return ERR_C_NOT_INIT;
	}	

	//Validate paremeters
	if ( !topic_id ){
		fprintf(stderr,"tc_client_unbind_rx() : INVALID PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Get in requests queue
	tc_client_get_server_access();

	//Lock topic database
	tc_client_db_lock();

	//Check if its registered as consumer of this topic 
	if ( !(topic = tc_client_db_topic_search( topic_id )) || !topic->is_consumer ){
		fprintf(stderr,"tc_client_unbind_rx() : NOT REGISTERED AS CONSUMER OF TOPIC ID %u\n",topic_id);
		tc_client_db_unlock();
		tc_client_release_server_access();
		return ERR_NODE_NOT_REG_RX;
	}

	//Check if node is bound to topic as consumer
	if ( !topic->is_rx_bound ){
		DEBUG_MSG_TC_CLIENT("tc_client_unbind_rx() : Node not bound to topic ID %u as consumer\n",topic_id);
		tc_client_db_unlock();
		tc_client_release_server_access();
		return ERR_OK;	
	}

	//Prepare request
	memset(&msg,0,sizeof(NET_MSG));

	msg.type = REQ_MSG;
	msg.op = UNBIND_RX;
	msg.node_ids[0] = tc_node_id;
	msg.n_nodes = 1;
	msg.topic_id = topic_id;

	//Unlock topic database
	tc_client_db_unlock();

	//Send request
	if ( tc_network_send_msg( &server_sock, &msg, NULL ) ){
		fprintf(stderr,"tc_client_unbind_rx() : ERROR SENDING REQUEST FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_SEND_REQUEST;
	}
	
	DEBUG_MSG_TC_CLIENT("tc_client_unbind_rx() : Waiting for topic id %u request response\n",topic_id);

	//Get answer
	memset(&msg,0,sizeof(NET_MSG));

	if ( tc_network_get_msg( &server_sock, C_REQUESTS_TIMEOUT, &msg, NULL ) ){
		fprintf(stderr,"tc_client_unbind_rx() : ERROR RECEIVING REQUEST ANSWER FOR TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_GET_ANSWER;
	}

	//Check if unbind was successfull
	if ( msg.type != ANS_MSG || msg.error || msg.node_ids[0] != tc_node_id ){
		fprintf(stderr,"tc_client_unbind_rx() : SERVER DENIED UNBIND AS CONSUMER FROM TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return msg.error;
	}

	//Lock topic database
	tc_client_db_lock();

	//Check if its registered as producer of this topic 
	if ( !(topic = tc_client_db_topic_search( topic_id )) || !topic->is_consumer ){
		fprintf(stderr,"tc_client_unbind_rx() : NOT REGISTERED AS PRODUCER OF TOPIC ID %u\n",topic_id);
		tc_client_db_unlock();
		tc_client_release_server_access();
		return ERR_NODE_NOT_REG_RX;
	}

	//Unlock DB (required for management module)
	tc_client_db_unlock();

	//Wait for unbind -- Management module will receive a request from the server
	while( topic->is_rx_bound && tries-- )
		usleep(1000);

	if ( tries <= 0 ){
		fprintf(stderr,"tc_client_unbind_rx() : TIMEDOUT WHILE WAITING FOR UNBIND ON TOPIC ID %u\n",topic_id);
		tc_client_release_server_access();
		return ERR_UNBIND_RX_TIMEDOUT;
	}

	tc_client_release_server_access();

	DEBUG_MSG_TC_CLIENT("tc_client_unbind_rx() Unbound as consumer from topic id %u\n",topic_id);

	return ERR_OK;
}

int tc_client_topic_send( unsigned int topic_id , char *data, int data_size )
{
	DEBUG_MSG_TC_CLIENT("tc_client_topic_send() TOPIC ID %u ...\n",topic_id);

	SOCK_ENTITY sock;
	char *temp = NULL;
	int len = 0, total = 0;
	int ret = -1, seq_n = 0;
	TOPIC_C_ENTRY *topic = NULL;

	if ( !init ){
		fprintf(stderr,"tc_client_topic_send() : MODULE IS NOT INITIALIZED\n");
		return ERR_C_NOT_INIT;
	}	

	//Validate parameters
	if ( !topic_id || !data || !data_size ){
		fprintf(stderr,"tc_client_topic_send() : INVALID PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Get topic entry
	tc_client_db_lock();

	if ( !(topic = tc_client_db_topic_search(topic_id) ) || !topic->is_producer ){
		fprintf(stderr,"tc_client_topic_send() : NOT REGISTERED AS PRODUCER OF TOPIC ID %u\n",topic_id);
		tc_client_db_unlock();
		return ERR_NODE_NOT_REG_TX;
	}

	tc_client_db_unlock();

	//Check if node is bound to topic as producer
	if ( !topic->is_tx_bound ){
		fprintf(stderr,"tc_client_topic_send() : NODE NOT BOUND TO TOPIC ID %u AS PRODUCER\n",topic_id);
		return ERR_NODE_NOT_BOUND_TX;	
	}

	//Lock send mutex
	tc_client_lock_topic_tx( topic, 0 );

	//Note : We could got blocked on mutex while channel was being destroyed or unbound by the management module
	//Check if channel being destroyed
	if ( topic->is_closing ){
		fprintf(stderr,"tc_client_topic_send() : TOPIC ID %u IS BEING CLOSED\n",topic_id);
		tc_client_unlock_topic_tx( topic );
		return ERR_TOPIC_CLOSING;
	}

	//Check if message fits in topic
	if ( data_size > topic->channel_size ){
		fprintf(stderr,"tc_client_topic_send() : MESSAGE SIZE (%d) TOO BIG FOR TOPIC ID %d (SIZE %d)\n",data_size,topic->topic_id,topic->channel_size);
		tc_client_unlock_topic_tx( topic );
		return ERR_DATA_SIZE;
	}

	//Alloc temp mem (8 extra char for order stamps and data size)
	if ( !(temp = (char *) malloc(D_MTU+8)) ){
		fprintf(stderr,"tc_client_topic_send() : NOT ENOUGH MEMORY TO SEND DATA TO TOPIC %u\n",topic_id);
		tc_client_unlock_topic_tx( topic );
		return ERR_MEM_MALLOC;			
	}

	sock = topic->topic_sock;

	len = data_size;//Remaining number of bytes to send

	//Split and send data according to MTU 
	while( len > D_MTU ){
	
		*((int *)temp) = seq_n;
		*(((int *)temp)+1) = data_size;
		memcpy( temp+8, data + (data_size - len), D_MTU );

		if ( (ret = sock_send( &sock, NULL, temp, D_MTU+8 )) <= 0 ){
			fprintf(stderr,"tc_client_topic_send() : ERROR SENDING DATA TO TOPIC %u\n",topic_id);
			perror("tc_client_topic_send() : ");

			if ( topic->is_updating ){
				//Topic updating ( probably its both consumer and producer and is being unregistered as consumer )
				//This data is invalid now (wont receive the remaining)
				ret = ERR_TOPIC_IN_UPDATE;
			}

			free(temp);
			tc_client_unlock_topic_tx( topic );
			return ret;
		}

		len = len - D_MTU;
		total = total + D_MTU;
		seq_n++;
	}

	//Send remaining data ( last fragment with size < MTU )
	if ( len > 0 ){

		*((char *)temp) = seq_n;
		*(((int *)temp)+1) = data_size;
		memcpy( temp+8, data + (data_size - len), len );
		
		if ( (ret = sock_send( &sock, NULL, temp, len+8 )) <= 0 ){
			fprintf(stderr,"tc_client_topic_send() : ERROR SENDING DATA TO TOPIC %u\n",topic_id);
			perror("tc_client_topic_send() : ");

			if ( topic->is_updating ){
				//Topic updating ( probably its both consumer and producer and is being unregistered as consumer )
				//This data is invalid now (wont send the remaining)
				ret = ERR_TOPIC_IN_UPDATE;
			}

			free(temp);
			tc_client_unlock_topic_tx( topic );
			return ret;
		}

		total = total + len;
	}
	
	free(temp);
	
	tc_client_unlock_topic_tx( topic );

	DEBUG_MSG_TC_CLIENT("tc_client_topic_send() Sent %u bytes to topic Id %u\n",total,topic_id);

	return total;	
}

int tc_client_topic_receive( unsigned int topic_id, unsigned int timeout, char *ret_data )
{
	DEBUG_MSG_TC_CLIENT("tc_client_topic_receive() TOPIC ID %u ...\n",topic_id);

	int ret = -1;
	TOPIC_C_ENTRY *topic = NULL;
	int len = 0, data_size = 0;
	unsigned int wait = timeout;
	SOCK_ENTITY sock, unblock_sock;
	char *temp = NULL, seq_n = 0, got_first = 0;

	if ( !init ){
		fprintf(stderr,"tc_client_topic_receive() : MODULE IS NOT INITIALIZED\n");
		return ERR_C_NOT_INIT;
	}	

	//Validate parameters
	if ( !topic_id || !ret_data ){
		fprintf(stderr,"tc_client_topic_receive() : INVALID PARAMETERS\n");
		return ERR_INVALID_PARAM;
	}

	//Get topic entry
	tc_client_db_lock();

	if ( !(topic = tc_client_db_topic_search(topic_id) ) || !topic->is_consumer ){
		fprintf(stderr,"tc_client_topic_receive() : NOT REGISTERED AS CONSUMER OF TOPIC ID %u\n",topic_id);
		tc_client_db_unlock();
		return ERR_NODE_NOT_REG_RX;
	}

	tc_client_db_unlock();

	//Check if node is bound to topic as consumer
	if ( !topic->is_rx_bound ){
		fprintf(stderr,"tc_client_topic_receive() : NODE NOT BOUND TO TOPIC ID %u AS CONSUMER\n",topic_id);
		return ERR_NODE_NOT_BOUND_RX;	
	}

	//Lock receive mutex
	tc_client_lock_topic_rx( topic, wait );
		
	//Note : We could got blocked on mutex while channel was being destroyed by the management module
	//Check if channel being destroyed
	if ( topic->is_closing ){
		fprintf(stderr,"tc_client_topic_receive(): TOPIC ID %u IS BEING CLOSED\n",topic_id);
		tc_client_unlock_topic_rx( topic );
		return ERR_TOPIC_CLOSING;
	}

	//Several MTU fragments can be received so we need to collect all of them and restore original data
	//Alloc temp mem (8 extra char for order stamps and data size)
	if ( (temp = (char *) malloc(D_MTU+8)) == NULL ){
		fprintf(stderr,"tc_client_topic_receive() : NOT ENOUGH MEMORY TO RECEIVE DATA FROM TOPIC %u\n",topic_id);
		tc_client_unlock_topic_rx( topic );
		return ERR_MEM_MALLOC;			
	}

	sock = topic->topic_sock;
	unblock_sock = topic->unblock_rx_sock;

	do{
		//Get first message fragment and get total message size
		if ( (ret = sock_receive( &sock, &unblock_sock, wait, temp, 0, NULL )) < 0){

			//Received unblock signal -- Check if node was unbound/unregistered from topic
			if ( (ret == ERR_DATA_UNBLOCK) && topic->is_closing ){
				fprintf(stderr,"tc_client_topic_receive() : UNBLOCK -- TOPIC ID %u IS BEING CLOSED\n",topic_id);
				tc_client_unlock_topic_rx( topic );
				return ERR_TOPIC_CLOSING;

			}else	if ( (ret == ERR_DATA_UNBLOCK) && !topic->is_rx_bound ){
				fprintf(stderr,"tc_client_topic_receive() : UNBLOCK -- NODE NOT BOUND TO TOPIC ID %u AS CONSUMER\n",topic_id);
				tc_client_unlock_topic_rx( topic );
				return ERR_NODE_NOT_BOUND_RX;

			}else	if ( (ret == ERR_DATA_UNBLOCK) && !topic->is_consumer ){
				fprintf(stderr,"tc_client_topic_receive() : UNBLOCK -- NOT REGISTERED AS CONSUMER OF TOPIC ID %u\n",topic_id);
				tc_client_unlock_topic_rx( topic );
				return ERR_NODE_NOT_REG_RX;

			}else if ( ret == ERR_DATA_UNBLOCK ){
				//Probably an unread unlock message from other threads? Ignore it
				continue;
			}

			if ( topic->is_updating ){
				//Topic updating ( probably its both consumer and producer and is being unregistered as producer )
				//This data is invalid now (wont receive the remaining)
				fprintf(stderr,"tc_client_topic_receive() : UNBLOCK -- TOPIC ID %u UPDATING\n",topic_id);
				ret = ERR_TOPIC_IN_UPDATE;
			}

			//Other error during receive
			fprintf(stderr,"tc_client_topic_receive() : ERROR RECEIVING DATA FROM TOPIC %u\n",topic_id);
			free(temp);
			tc_client_unlock_topic_rx( topic );
			return ret;
		}
 
		seq_n = *((int *)temp);

		//Its possible to receive old fragments from other messages when a producer tries to send at a rate faster than the negotiated
		//In this case some fragments got queued at the producer and the consumers timed-out while receiving
		//Discard this fragments and wait for the first of the new message
		if ( seq_n && !got_first){
			printf("tc_client_topic_receive() : Received old fragment (%d) on topic_id %u\n",seq_n,topic_id);
			continue;
		}

		if ( !seq_n && !got_first ){
			got_first = 1;
			data_size = *(((int *)temp)+1);
		}

		//Copy data from temp buffer to app buffer and align
		memcpy( ret_data+(seq_n*D_MTU), temp+8, ret-8 );
		
		len = len + ret-8;

		//Timeout to receive further fragments (ms)
   		wait = FRAG_TIMEOUT;

	}while( (data_size - len) > 0 );
		
	free(temp);

	tc_client_unlock_topic_rx( topic );

	DEBUG_MSG_TC_CLIENT("tc_client_topic_receive() Received %u bytes from topic Id %u\n",len,topic_id);

	return len;
}

static int tc_client_comm_init( void )
{
	DEBUG_MSG_TC_CLIENT("tc_client_comm_init() ...\n");

	if ( !server.port ){
		//Server is in the same local node

		//Create local server socket
		if ( sock_open( &server_sock, LOCAL ) < 0 ){
			fprintf(stderr,"tc_client_comm_init() : ERROR CREATING LOCAL SERVER SOCKET\n");
			return ERR_SOCK_CREATE;
		}

		//Set and bind to host address
		NET_ADDR host = {CLIENT_AC_LOCAL_FILE, 0};
		if ( sock_bind( &server_sock, &host ) ){
			fprintf(stderr,"tc_client_comm_init() : ERROR BINDING SOCKET TO HOST ADDRESS\n");
			sock_close(&server_sock);
			return ERR_SOCK_BIND_HOST;
		}

		//Connect socket to server
		NET_ADDR peer = {SERVER_AC_LOCAL_FILE,0};
		if ( sock_connect_peer( &server_sock, &peer ) ){
			fprintf(stderr,"tc_client_comm_init() : ERROR CONNECTING LOCAL SOCKET TO SERVER\n");
			sock_close ( &server_sock );
			return ERR_SOCK_BIND_PEER;
		}

	}else{
		//Server is in a remote node

		//Create remote server socket
		if ( sock_open( &server_sock, REMOTE_UDP ) < 0){
			fprintf(stderr,"tc_client_comm_init() : ERROR CREATING SERVER SOCKET\n");
			return ERR_SOCK_CREATE;
		}

		//Set and bind to host address
		NET_ADDR host;
		strcpy(host.name_ip, nic_ip);
		host.port = server.port;

		if ( sock_bind( &server_sock, &host ) < 0){
			fprintf(stderr,"tc_client_comm_init() : ERROR BINDING SOCKET TO LOCAL ADDRESS\n");
			sock_close( &server_sock );
			return ERR_SOCK_BIND_HOST;
		}

		//Set remote address
		NET_ADDR peer;
		strcpy(peer.name_ip, server.name_ip);
		peer.port = server.port;

		//Connect to server
		if ( sock_connect_peer( &server_sock, &peer ) ){
			fprintf(stderr,"tc_client_comm_init() : ERROR CONNECTING TO SERVER -- WRONG SERVER IP/PORT?\n");
			sock_close( &server_sock );
			return ERR_SOCK_BIND_PEER;
		}
	}

	DEBUG_MSG_TC_CLIENT("tc_client_comm_init() Comunication link with server created\n");

	return ERR_OK;
}

static int tc_client_comm_close( void )
{
	DEBUG_MSG_TC_CLIENT("tc_client_comm_close() ...\n");

	if ( sock_close( &server_sock ) ){
		fprintf(stderr,"tc_client_comm_close() : ERROR CLOSING SERVER SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	DEBUG_MSG_TC_CLIENT("tc_client_comm_close() Comunication link closed\n");

	return ERR_OK;
}

static int tc_client_node_reg ( unsigned int node_id, unsigned int *ret_node_id )
{
	DEBUG_MSG_TC_CLIENT("tc_client_node_reg() Node Id %u...\n",node_id);

	NET_MSG msg;

	if ( init ){
		fprintf(stderr,"tc_client_node_reg() : MODULE RUNNING -> ALREADY REGISTERED\n");
		return ERR_OK;
	}	

	//Prepare request
	memset(&msg,0,sizeof(NET_MSG));

	msg.type = REQ_MSG;
	msg.op = REG_NODE;
	msg.node_ids[0] = tc_node_id;
	msg.n_nodes = 1;

	//Send request
	if ( tc_network_send_msg( &server_sock, &msg, NULL ) ){
		fprintf(stderr,"tc_client_node_reg() : ERROR SENDING REQUEST FOR NODE ID %u\n",node_id);
		return ERR_SEND_REQUEST;
	}

	DEBUG_MSG_TC_CLIENT("tc_client_node_reg() : Waiting for node id %u registration request response\n",node_id);

	//Get answer
	if ( tc_network_get_msg( &server_sock, C_REQUESTS_TIMEOUT, &msg, NULL ) ){
		fprintf(stderr,"tc_client_node_reg() : ERROR RECEIVING REQUEST ANSWER FOR NODE ID %u\n",node_id);
		return ERR_GET_ANSWER;
	}

	//Check if operation was successfull
	if( msg.type != ANS_MSG || msg.error ){
		fprintf(stderr,"tc_client_node_reg() : SERVER DENIED REGISTRATION OF NODE ID %u\n",node_id);
		return msg.error;
	}

	*ret_node_id = msg.node_ids[0];

	DEBUG_MSG_TC_CLIENT("tc_client_node_reg() NODE ID %u RETURNING 0\n",node_id);

	return ERR_OK;
}

static int tc_client_node_unreg ( void )
{
	DEBUG_MSG_TC_CLIENT("tc_client_node_unreg() ...\n");

	NET_MSG msg;

	if ( !init ){
		fprintf(stderr,"tc_client_node_unreg() : MODULE IS NOT RUNNING\n");
		return ERR_C_NOT_INIT;
	}	

	//Prepare request
	memset(&msg,0,sizeof(NET_MSG));

	msg.type = REQ_MSG;
	msg.op = UNREG_NODE;
	msg.node_ids[0] = tc_node_id;
	msg.n_nodes = 1;
	
	//Get in requests queue
	tc_client_get_server_access();

	//Send request
	if ( tc_network_send_msg( &server_sock, &msg, NULL ) ){
		fprintf(stderr,"tc_client_node_unreg() : ERROR SENDING REQUEST FOR NODE ID %u\n",tc_node_id);
		tc_client_release_server_access();
		return ERR_SEND_REQUEST;
	}
	
	DEBUG_MSG_TC_CLIENT("tc_client_node_unreg() : Waiting for node id %u unregistration request response\n",tc_node_id);

	//Get answer
	memset(&msg,0,sizeof(NET_MSG));

	if ( tc_network_get_msg( &server_sock, C_REQUESTS_TIMEOUT, &msg, NULL ) ){
		fprintf(stderr,"tc_client_node_unreg() : ERROR RECEIVING REQUEST ANSWER FOR NODE ID %u\n",tc_node_id);
		tc_client_release_server_access();
		return ERR_GET_ANSWER;
	}

	//Check if operation was successfull
	if( msg.type != ANS_MSG || msg.error || msg.node_ids[0] != tc_node_id ){
		fprintf(stderr,"tc_client_node_unreg() : SERVER DENIED REGISTRATION OF NODE ID %u\n",tc_node_id);
		tc_client_release_server_access();
		return msg.error;
	}

	tc_client_release_server_access();

	DEBUG_MSG_TC_CLIENT("tc_client_node_unreg() NODE ID %u RETURNING 0\n",tc_node_id);

	return ERR_OK;
}

static int tc_client_modules_init( void )
{
	DEBUG_MSG_TC_CLIENT("tc_client_modules_init() ...\n");

	//Start discovery module
	if ( tc_client_discovery_init( nic_ifface ) ){
		fprintf(stderr,"tc_client_modules_init() : ERROR STARTING DISCOVERY MODULE\n");
		return ERR_DISCOVERY_INIT;
	}

	//Discover server 
	if ( tc_client_discovery_find_server( DISCOVERY_TIMEOUT, &server ) ){
		fprintf(stderr,"tc_client_modules_init() : COULDN'T DISCOVER SERVER\n");
		return ERR_DISCOVERY_SERVER;
	}

	printf("tc_client_modules_init() : DISCOVERED SERVER %s:%u\n",server.name_ip,server.port);

	//Create comunication link and connect to server
	if ( tc_client_comm_init() < 0 ){
		fprintf(stderr,"tc_client_modules_init() : ERROR INITIALIZING SERVER COMM LINK\n");
		tc_client_modules_close();
		return ERR_COMM_INIT;
	}

	//Start client database module
	if ( tc_client_db_init() ){
		fprintf(stderr,"tc_client_modules_init() : ERROR STARTING CLIENT DATABASE MODULE\n");
		tc_client_modules_close();
		return ERR_DB_INIT;
	}
	
	//Register node in server
	if ( tc_client_node_reg( tc_node_id, &tc_node_id ) ){
		fprintf(stderr,"tc_client_modules_init() : ERROR REGISTERING NODE ID %u IN SERVER\n",tc_node_id);
		tc_client_modules_close();
		return ERR_REG_NODE;
	}

	//Start monitoring module
	if ( tc_client_monit_init( nic_ifface, tc_node_id, &server ) ){
		fprintf(stderr,"tc_client_modules_init() : ERROR STARTING MONITORING MODULE\n");
		tc_client_modules_close();
		return ERR_MONIT_INIT;
	}

	//Start reservation module
	if ( tc_client_reserv_init( nic_ifface, tc_node_id, &server ) ){
		fprintf(stderr,"tc_client_modules_init() : ERROR STARTING RESERVATION MODULE\n");
		tc_client_modules_close();
		return ERR_RESERV_INIT;
	}
	
	//Start management module
	if ( tc_client_management_init( nic_ifface, tc_node_id, &server ) ){
		fprintf(stderr,"tc_client_modules_init() : ERROR STARTING MANAGEMENT MODULE\n");
		tc_client_modules_close();
		return ERR_MANAG_INIT;
	}

	//Start notifications module
	if ( tc_client_notifications_init( nic_ifface, &server ) ){
		fprintf(stderr,"tc_client_modules_init() : ERROR STARTING NOTIFICATIONS MODULE\n");
		tc_client_modules_close();
		return ERR_NOTIFIC_INIT;
	}

	DEBUG_MSG_TC_CLIENT("tc_client_modules_init() All client modules initialized\n");

	return ERR_OK;
}

static int tc_client_modules_close( void )
{
	DEBUG_MSG_TC_CLIENT("tc_client_modules_close() ...\n");

	int ret;

	if ( (ret = tc_client_management_close()) && ret != ERR_C_NOT_INIT ){
		fprintf(stderr,"tc_client_modules_close() : ERROR CLOSING MANAGEMENT MODULE\n");
		return ERR_MANAG_CLOSE;
	}

	if ( (ret = tc_client_reserv_close()) && ret != ERR_C_NOT_INIT ){
		fprintf(stderr,"tc_client_modules_close() : ERROR CLOSING RESERVATION MODULE\n");
		return ERR_RESERV_CLOSE;
	}

	if ( (ret = tc_client_monit_close()) && ret != ERR_C_NOT_INIT ){
		fprintf(stderr,"tc_client_modules_close() : ERROR CLOSING MONITORING MODULE\n");
		return ERR_MONIT_CLOSE;
	}

	if ( (ret = tc_client_node_unreg()) && ret != ERR_C_NOT_INIT ){
		fprintf(stderr,"tc_client_modules_close() : ERROR UNREGISTERING NODE\n");
		return ERR_UNREG_NODE;
	}

	if ( (ret = tc_client_db_close()) && ret != ERR_C_NOT_INIT ){
		fprintf(stderr,"tc_client_modules_close() : ERROR CLOSING CLIENT DATABASE MODULE\n");
		return ERR_DB_CLOSE;
	}

	if ( ( ret = tc_client_notifications_close()) && ret != ERR_C_NOT_INIT ){
		fprintf(stderr,"tc_client_modules_close() : ERROR CLOSING NOTIFICATIONS MODULE\n");
		return ERR_NOTIFIC_CLOSE;
	}

	if ( ( ret = tc_client_discovery_close()) && ret != ERR_C_NOT_INIT ){
		fprintf(stderr,"tc_client_modules_close() : ERROR CLOSING DISCOVERY MODULE\n");
		return ERR_DISCOVERY_CLOSE;
	}

	if ( tc_client_comm_close() ){
		fprintf(stderr,"tc_client_modules_close() : ERROR CLOSING COMUNICATIONS MODULE\n");
		return ERR_COMM_CLOSE;
	}

	DEBUG_MSG_TC_CLIENT("tc_client_modules_close() Closed all active modules\n");

	return ERR_OK;
}

static int tc_client_get_server_access( void )
{
	DEBUG_MSG_TC_CLIENT("tc_client_get_server_access() ...\n");

	int ret;

	if ( !init ){
		fprintf(stderr,"tc_client_get_server_access() : MODULE IS NOT INITIALIZED\n");
		return ERR_C_NOT_INIT;
	}

	ret = pthread_mutex_lock(&server_lock);

	if ( ret == EOWNERDEAD ){
		fprintf(stderr,"tc_client_get_server_access() : PREVIOUS HOLDING THREAD TERMINATED WHILE HOLDING MUTEX LUCK");
		pthread_mutex_consistent(&server_lock);
	}

	if ( ret == EAGAIN ){
		fprintf(stderr,"tc_client_get_server_access() : MAX NUMBER RECURSIVE LOCKS EXCEEDED");
		return -1;
	}
		
	if ( ret == EINVAL ){
		fprintf(stderr,"tc_client_get_server_access() : CALLING THREAD PRIORITY HIGHER THAN MUTEX PRIORITY/MUTEX NOT INITIALIZED");
		return -2;
	}
	
	if ( ret == ENOTRECOVERABLE ){
		fprintf(stderr,"tc_client_get_server_access() : MUTEX IS NOT RECOVERABLE");
		return -3;
	}

	if ( ret == EDEADLK ){
		fprintf(stderr,"tc_client_get_server_access() : CURRENT THREAD ALREADY OWNS THE MUTEX");
		return ERR_OK;
	}

	if ( ret == EFAULT ){
		fprintf(stderr,"tc_client_get_server_access() : INVALID MUTEX POINTER");
		return -6;
	}

	if ( ret == ENOTRECOVERABLE ){
		fprintf(stderr,"tc_client_release_server_access() : MUTEX IS NOT RECOVERABLE");
		return -7;
	}

	DEBUG_MSG_TC_CLIENT("tc_client_release_server_access() Got server access\n");

	return ERR_OK; 
}

static int tc_client_release_server_access( void )
{
	DEBUG_MSG_TC_CLIENT("tc_client_release_server_access() ...\n");

	int ret;

	if ( !init ){
		fprintf(stderr,"tc_client_release_server_access() : MODULE IS NOT INITIALIZED\n");
		return ERR_C_NOT_INIT;
	}

	ret = pthread_mutex_unlock(&server_lock);

	if ( ret == EAGAIN ){
		fprintf(stderr,"tc_client_release_server_access() : MAX NUMBER RECURSIVE LOCKS EXCEEDED");
		return -1;
	}
		
	if ( ret == EINVAL ){
		fprintf(stderr,"tc_client_release_server_access() : CALLING THREAD PRIORITY HIGHER THAN MUTEX PRIORITY/MUTEX NOT INITIALIZED");
		return -2;
	}

	if ( ret == EPERM ){
		fprintf(stderr,"tc_client_release_server_access() : THREAD DOES NOT OWN THE MUTEX");
		return -3;
	}

	DEBUG_MSG_TC_CLIENT("tc_client_release_server_access() Released server access\n");

	return ERR_OK; 
}
