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

/**	@file TC_Server_Management.c
*	@brief Source code of the functions for the server management module
*
*	This file contains the implementation of the functions for the server management module. 
*	This module is an auxiliary module for the servers admission control module. This module issues the necessary
*	control operations and the necessary requests to the clients for the node removal,topic update and bind/unbind check routines.
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
#include <assert.h>

#include "Sockets.h"
#include "TC_Utils.h"
#include "TC_Config.h"
#include "TC_Server_DB.h"
#include "TC_Data_Types.h"
#include "TC_Error_Types.h"
#include "TC_Server_Management.h"

/**	@def DEBUG_MSG_SERVER_MNG
*	@brief If "ENABLE_DEBUG_SERVER_MANAGEMENT" is defined debug messages related to this module are printed
*/
#if ENABLE_DEBUG_SERVER_MANAGEMENT
#define DEBUG_MSG_SERVER_MNG(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG_SERVER_MNG(...)
#endif

static char init = 0;

static NET_ADDR server_addr;

//Sockets to send multi requests
static SOCK_ENTITY req_local_sock,req_remote_sock;

//Sockets to receive individual nodes replies
//Could have used the same loca req socket for local answers since we only supp one node but to keep things mirroed I use a different one 
static SOCK_ENTITY ans_local_sock,ans_remote_sock;

static int tc_server_management_open_req_sock( void );
static int tc_server_management_open_ans_sock( void );
static int tc_server_management_close_req_sock( void );
static int tc_server_management_close_ans_sock( void );
	
//Sends topic related requests to nodes (binds,unbinds,del topic, modify topic properties)
static int topic_multi_op_request( NODE_BIND_ENTRY *node_list[], unsigned int n_nodes, TOPIC_ENTRY *topic, unsigned char op_type, NODE_BIND_ENTRY *ret_err_list[], unsigned int *ret_n_err );

int tc_server_management_init( NET_ADDR *server_remote )
{
	DEBUG_MSG_SERVER_MNG("tc_server_management_init() ...\n");

	if ( init ){
		fprintf(stderr,"tc_server_management_init() : MODULE ALREADY INITIALIZED\n");
		return ERR_S_ALREADY_INIT;
	}

	assert( server_remote );
	assert( server_remote->name_ip );
	assert( server_remote->port );

	//Set server remote address
	strcpy(server_addr.name_ip,server_remote->name_ip);
	server_addr.port = server_remote->port;

	//Create request sockets
	if ( tc_server_management_open_req_sock() ){
		fprintf(stderr,"tc_server_management_init() : ERROR CREATING REQUEST SOCKETS\n");
		return ERR_SOCK_CREATE;
	}

	//Create reply sockets
	if ( tc_server_management_open_ans_sock() ){
		fprintf(stderr,"tc_server_management_init() : ERROR CREATING REPLY SOCKETS\n");
		return ERR_SOCK_CREATE;
	}

	init = 1;

	DEBUG_MSG_SERVER_MNG("tc_server_management_init() Returning 0\n");

	return ERR_OK;
}

int tc_server_management_close( void )
{
	DEBUG_MSG_SERVER_MNG("tc_server_management_close() ...\n");

	if ( !init ){
		fprintf(stderr,"tc_server_management_close() : MODULE ISNT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	//Prevent further module access
	init = 0;

	if ( tc_server_management_close_req_sock() ){
		fprintf(stderr,"tc_server_management_close() : ERROR CLOSING REQUEST SOCKETS\n");
		return ERR_SOCK_CLOSE;
	}

	if ( tc_server_management_close_ans_sock() ){
		fprintf(stderr,"tc_server_management_close() : ERROR CLOSING REPLY SOCKETS\n");
		return ERR_SOCK_CLOSE;
	}

	DEBUG_MSG_SERVER_MNG("tc_server_management_close() Returning 0\n");

	return ERR_OK;
}

int tc_server_management_reserv_req( NODE_ENTRY *node, TOPIC_ENTRY *topic, unsigned char tc_request, unsigned int req_load )
{
	DEBUG_MSG_SERVER_MNG("tc_server_management_reserv_req() ...\n");

	NET_ADDR client;
	NET_MSG request;
	NET_MSG answer;

	if ( !init ){
		fprintf(stderr,"tc_server_management_reserv_req() : MODULE ISNT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	assert( node );
	assert( topic );
	assert( tc_request == TC_RESERV || tc_request == TC_FREE || tc_request == TC_MODIFY );

	//Prepare request msg
	memset(&request,0,sizeof(NET_MSG));

	request.type = REQ_MSG;
	request.op = tc_request;
	request.node_ids[0] = node->node_id;
	request.n_nodes = 1;
	request.topic_id = topic->topic_id;
	request.topic_load = req_load;
	request.topic_addr = topic->address;

	//Set client address
	strcpy(client.name_ip, MANAGEMENT_GROUP_IP);
	client.port = MANAGEMENT_GROUP_PORT;

	if ( !node->address.port ){
		//Client is in the same local node
		strcpy(client.name_ip, CLIENT_MANAGEMENT_REQ_LOCAL_FILE);
		tc_network_send_msg( &req_local_sock, &request, &client );
	
		DEBUG_MSG_SERVER_MNG("tc_server_management_reserv_req() : Waiting for topic id %u resource reservation on node ID %u local request response\n",topic->topic_id,node->node_id);
		tc_network_get_msg( &ans_local_sock, S_REQUESTS_TIMEOUT, &answer, NULL );
	}else{
		//Client is in a remote node
		tc_network_send_msg( &req_remote_sock, &request, &client );
	
		DEBUG_MSG_SERVER_MNG("tc_server_management_reserv_req() : Waiting for topic id %u resource reservation on node ID %u remote request response\n",topic->topic_id,node->node_id);
		tc_network_get_msg( &ans_remote_sock, S_REQUESTS_TIMEOUT, &answer, NULL );
	}

	//Check if operation was successfull
	if( answer.type != ANS_MSG || answer.error || answer.node_ids[0] != node->node_id ){
		fprintf(stderr,"tc_server_management_reserv_req() : ERROR RESERVING BANDWIDTH FOR TOPIC ID %u ON NODE ID %u\n",topic->topic_id,node->node_id);
		return -3;
	}

	DEBUG_MSG_SERVER_MNG("tc_server_management_reserv_req() Reservation operation sucessfull\n");

	return ERR_OK;
}

int tc_server_management_rm_topic( TOPIC_ENTRY *topic )
{
	DEBUG_MSG_SERVER_MNG("tc_server_management_rm_topic() ...\n");

	NODE_BIND_ENTRY *prod_entry = NULL, *cons_entry = NULL;

	NODE_BIND_ENTRY *node_list[MAX_MULTI_NODES]; unsigned int n_nodes = 0;
	NODE_BIND_ENTRY *err_node_list[MAX_MULTI_NODES];unsigned int n_node_err = 0;

	if ( !init ){
		fprintf(stderr,"tc_server_management_rm_topic() : MODULE ISNT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic );

	//Filter all registered nodes to this topic
	memset(node_list,0,sizeof(node_list));
	n_nodes = 0;

	//Filter consumers
	for ( cons_entry = topic->cons_list; cons_entry ; cons_entry = cons_entry->next  ){
		//Check if this consumer is also producer of the same topic
		for ( prod_entry = topic->prod_list; prod_entry; prod_entry = prod_entry->next ){
			if ( cons_entry->node == prod_entry->node )
				//Also producer of same topic -- Don't list this node right now -- Leave it for when we pick producers (to avoid duplicates)
				break;
		}

		if ( !prod_entry ){
			//Node only consumer of this topic -> store it in list
			node_list[n_nodes] = cons_entry;
			n_nodes++;
		}

		//Check if we reached max number of nodes
		if ( n_nodes >= MAX_MULTI_NODES ){
			fprintf(stderr,"tc_server_management_rm_topic() : MAX NUMBER OF NODES REACHED\n");
			return ERR_NODE_MAX;	
		}
	}

	//Filter producers 
	for ( prod_entry = topic->prod_list; prod_entry; prod_entry = prod_entry->next ){
		node_list[n_nodes] = prod_entry;
		n_nodes++;
			
		//Check if we reached max number of nodes
		if ( n_nodes >= MAX_MULTI_NODES ){
			fprintf(stderr,"tc_server_management_rm_topic() : MAX NUMBER OF NODES REACHED\n");
			return ERR_NODE_MAX;	
		}
	}	

#if ENABLE_DEBUG_SERVER_MANAGEMENT
	int i;
	printf("\ntc_server_management_rm_topic() : node_list:\n");
	for ( i = 0; i < n_nodes; i++ ){
		printf("node_list[%d] = %d\n",i,node_list[i]->node->node_id);
	}
#endif

//what to do when some nodes fail to remove ?

	//Request nodes to delete topic (producers will also free reservations)
	if ( n_nodes && topic_multi_op_request( node_list, n_nodes, topic, DEL_TOPIC, err_node_list, &n_node_err ) ){
		fprintf(stderr,"tc_server_management_rm_topic() : ERROR REMOVING TOPIC ON ONE OR MORE NODES\n");
		return ERR_NODE_BIND;
	}

	DEBUG_MSG_SERVER_MNG("tc_server_management_rm_topic() Topic ID %u removed on all nodes 0\n",topic->topic_id);

	return ERR_OK;
}

int tc_server_management_set_topic( TOPIC_ENTRY *topic )
{
	DEBUG_MSG_SERVER_MNG("tc_server_management_set_topic() ...\n");

	NODE_BIND_ENTRY *prod_entry = NULL, *cons_entry = NULL;

	NODE_BIND_ENTRY *node_list[MAX_MULTI_NODES]; unsigned int n_nodes = 0;
	NODE_BIND_ENTRY *err_node_list[MAX_MULTI_NODES];unsigned int n_node_err = 0;

	if ( !init ){
		fprintf(stderr,"tc_server_management_set_topic() : MODULE ISNT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic );

	//Filter all registered nodes to this topic
	memset(node_list,0,sizeof(node_list));
	n_nodes = 0;

	//Filter consumers
	for ( cons_entry = topic->cons_list; cons_entry ; cons_entry = cons_entry->next  ){
		//Check if this consumer is also producer of the same topic
		for ( prod_entry = topic->prod_list; prod_entry; prod_entry = prod_entry->next ){
			if ( cons_entry->node == prod_entry->node )
				//Also producer of same topic -- Don't list this node right now -- Leave it for when we pick producers (to avoid duplicates)
				break;
		}

		if ( !prod_entry ){
			//Node only consumer of this topic -> store it in list
			node_list[n_nodes] = cons_entry;
			n_nodes++;
		}

		//Check if we reached max number of nodes
		if ( n_nodes >= MAX_MULTI_NODES ){
			fprintf(stderr,"tc_server_management_set_topic() : MAX NUMBER OF NODES REACHED\n");
			return ERR_NODE_MAX;	
		}
	}

	//Filter producers 
	for ( prod_entry = topic->prod_list; prod_entry; prod_entry = prod_entry->next ){
		node_list[n_nodes] = prod_entry;
		n_nodes++;
			
		//Check if we reached max number of nodes
		if ( n_nodes >= MAX_MULTI_NODES ){
			fprintf(stderr,"tc_server_management_set_topic() : MAX NUMBER OF NODES REACHED\n");
			return ERR_NODE_MAX;	
		}
	}

#if ENABLE_DEBUG_SERVER_MANAGEMENT
	int i;
	printf("\ntc_server_management_set_topic() : node_list:\n");
	for ( i = 0; i < n_nodes; i++ ){
		printf("node_list[%d] = %d\n",i,node_list[i]->node->node_id);
	}
#endif

//what to do when some nodes fail to update ?

	//Request nodes to update topic (producers will also update reservations)
	if ( n_nodes && topic_multi_op_request( node_list, n_nodes, topic, SET_TOPIC_PROP, err_node_list, &n_node_err ) ){
		fprintf(stderr,"tc_server_management_set_topic() : ERROR UPDATING TOPIC ON ONE OR MORE NODES\n");
		return ERR_NODE_BIND;
	}

	DEBUG_MSG_SERVER_MNG("tc_server_management_set_topic() Topic ID %d updated on all nodes\n",topic->topic_id);

	return ERR_OK;
}

int tc_server_management_rm_node( NODE_ENTRY *node )
{
	DEBUG_MSG_SERVER_MNG("tc_server_management_rm_node() ...\n");

	int aux = 0;
	TOPIC_ENTRY *topic = NULL;
	NODE_BIND_ENTRY *prod_entry = NULL, *cons_entry = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_management_rm_node() : MODULE ISNT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	assert( node );

#if ENABLE_DEBUG_SERVER_MANAGEMENT	
	printf("tc_server_management_rm_node() : DATABASE BEFORE REMOVING NODE\n");
	tc_server_db_topic_print();
	tc_server_db_node_print();
#endif
	//For debugging
	aux = node->node_id;

	//For each topic check if this node is producer/consumer and update all endpoint partners bandwidth
	for ( topic = tc_server_db_topic_get_first(); topic ; topic = topic->next ){

		//Check all producer node entries
		for ( prod_entry = topic->prod_list; prod_entry; prod_entry = prod_entry->next ){
			if ( prod_entry->node == node ){
				//This node is producer of this topic -- Update bandwidth of each consumer node of this topic
				for ( cons_entry = topic->cons_list; cons_entry; cons_entry = cons_entry->next ){
					cons_entry->node->downlink_load = cons_entry->node->downlink_load - topic->topic_load;
				}
				//Remove node as producer of this topic
				tc_server_db_topic_rm_prod_node( topic, prod_entry->node );
				break;			
			}
		}

		//Check all consumer node entries
		for ( cons_entry = topic->cons_list; cons_entry; cons_entry = cons_entry->next ){
			if ( cons_entry->node == node ){
				//This node is consumer of this topic -- Since we use multicast no need to update bandwidth of each producer node of this topic
				//Remove node as consumer of this topic
				tc_server_db_topic_rm_cons_node( topic, cons_entry->node );
				break;			
			}
		}

		//Check for possible unbinds
		if( tc_server_management_check_unbind( topic ) ){
			fprintf(stderr,"tc_server_management_rm_node() : ERROR INSIDE MANAGEMENT CHECK UNBIND OF TOPIC ID %u\n",topic->topic_id);
		}	
	}

	//Destroy node entry
	if ( tc_server_db_node_delete( node ) ){
		fprintf(stderr,"tc_server_management_rm_node() : ERROR REMOVING NODE ID %u ENTRY\n",aux);
		return -3;
	}

#if ENABLE_DEBUG_SERVER_MANAGEMENT
	printf("tc_server_management_rm_node() : DATABASE AFTER REMOVING NODE\n");
	tc_server_db_topic_print();
	tc_server_db_node_print();
#endif
	DEBUG_MSG_SERVER_MNG("tc_server_management_rm_node() Node id %u removed\n",aux);

	return ERR_OK;	
}

int tc_server_management_check_bind( TOPIC_ENTRY *topic )
{
	DEBUG_MSG_SERVER_MNG("tc_server_management_check_bind() ...\n");

	int i,j;
	char has_partner;

	NODE_BIND_ENTRY *prod_entry = NULL, *cons_entry = NULL;

	NODE_BIND_ENTRY *producers[MAX_MULTI_NODES]; unsigned int n_prod = 0;
	NODE_BIND_ENTRY *consumers[MAX_MULTI_NODES]; unsigned int n_cons = 0;

	NODE_BIND_ENTRY *rx_node_list[MAX_MULTI_NODES]; unsigned int rx_n_nodes = 0;
	NODE_BIND_ENTRY *tx_node_list[MAX_MULTI_NODES]; unsigned int tx_n_nodes = 0;
	NODE_BIND_ENTRY *err_node_list[MAX_MULTI_NODES];unsigned int n_node_err = 0;

	if ( !init ){
		fprintf(stderr,"tc_server_management_check_bind() : MODULE ISNT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic );

#if ENABLE_DEBUG_SERVER_MANAGEMENT
	printf("tc_server_management_check_bind() : DATABASE BEFORE BIND\n");
	tc_server_db_topic_print();
#endif

	//Filter producers list and get the entries of those which are bound or requesting a bind
	memset(producers,0,sizeof(producers));
	for ( n_prod = 0, prod_entry = topic->prod_list; prod_entry; prod_entry = prod_entry->next ){
		if ( prod_entry->is_bound || prod_entry->req_bind ){
			//Producer is bound or requesting a bind -> store its entry
			producers[n_prod] = prod_entry;
			n_prod++;
		}
			
		//Check if we reached max number of nodes
		if ( n_prod >= MAX_MULTI_NODES ){
			fprintf(stderr,"tc_server_management_check_bind() : MAX NUMBER OF PRODUCER NODES REACHED\n");
			return ERR_NODE_MAX;	
		}
	}	

	//Filter consumers list and get the entries of those which are bound or requesting a bind
	memset(consumers,0,sizeof(consumers));
	for ( n_cons = 0, cons_entry = topic->cons_list; cons_entry; cons_entry = cons_entry->next ){
		if ( cons_entry->is_bound || cons_entry->req_bind ){
			//Consumer is bound or requesting a bind -> store its entry
			consumers[n_cons] = cons_entry;
			n_cons++;
		}

		//Check if we reached max number of nodes
		if ( n_cons >= MAX_MULTI_NODES ){
			fprintf(stderr,"tc_server_management_check_bind() : MAX NUMBER OF CONSUMER NODES REACHED\n");
			return ERR_NODE_MAX;	
		}
	}
 
	DEBUG_MSG_SERVER_MNG("tc_server_management_check_bind() Going to check valid partners on topic id %u\n",topic->topic_id);

	//For each node on hold for a bind check if it has a valid partner (bound or also on hold)
	//NOTE: When one node is producer and consumer of the same topic, when it produces it produces only for the other consumers (no loopback)
	for ( i = 0; i < n_prod ; i++ ){
		has_partner = 0;

		for ( j = 0; j < n_cons; j ++){
			//Filter all consumer valid partner for this producer.A node which is consumer and producer of same topic is not a valid partnership (no loopback)
			if ( consumers[j]->node != producers[i]->node ){
				//Bind consumer partner
				if ( !consumers[j]->is_bound ){
					printf("tc_server_management_check_bind() : going to bind node %s as rx to topic %d\n",consumers[j]->node->address.name_ip,topic->topic_id);
					rx_node_list[rx_n_nodes] = consumers[j];
					rx_n_nodes++;
				}
				has_partner = 1;
			}	
		}

		//Bind this producer if it has a valid consumer partner
		if ( !producers[i]->is_bound && has_partner ){
			printf("tc_server_management_check_bind() : going to bind node %s as tx to topic %d\n",producers[i]->node->address.name_ip,topic->topic_id);
			tx_node_list[tx_n_nodes] = producers[i];
			tx_n_nodes++;
		}	
	}

#if ENABLE_DEBUG_SERVER_MANAGEMENT
	printf("\tc_server_management_check_bind() : tx_node_list:\n");
	for ( i = 0; i < tx_n_nodes; i++ ){
		printf("tx_node_list[%d] = %d\n",i,tx_node_list[i]->node->node_id);
	}

	printf("tc_server_management_check_bind() : rx_node_list:\n");
	for ( i = 0; i < rx_n_nodes; i++ ){
		printf("rx_node_list[%d] = %d\n",i,rx_node_list[i]->node->node_id);
	}
#endif

//what to do when some nodes fail to bind ?

	//Send bind request to all the consumer nodes(if any)
	if ( rx_n_nodes && topic_multi_op_request( rx_node_list, rx_n_nodes, topic, BIND_RX, err_node_list, &n_node_err ) ){
		fprintf(stderr,"tc_server_management_check_bind() : ERROR BINDING ONE OR MORE CONSUMER NODES\n");
		return ERR_NODE_BIND;
	}

	//Send bind request to all the producer nodes
	if ( tx_n_nodes && topic_multi_op_request( tx_node_list, tx_n_nodes, topic, BIND_TX, err_node_list, &n_node_err ) ){
		fprintf(stderr,"tc_server_management_check_bind() : ERROR BINDING ONE OR MORE PRODUCER NODES\n");
		return ERR_NODE_BIND;
	}

	//Update database and node status
	for ( i = 0; i < rx_n_nodes ; i++ ){
		rx_node_list[i]->is_bound = 1;
		rx_node_list[i]->req_bind = 0;
	}

	for ( i = 0; i < tx_n_nodes ; i++ ){
		tx_node_list[i]->is_bound = 1;
		tx_node_list[i]->req_bind = 0;
	}

#if ENABLE_DEBUG_SERVER_MANAGEMENT
	printf("tc_server_management_check_bind() : DATABASE AFTER BIND\n");
	tc_server_db_topic_print();
#endif

	DEBUG_MSG_SERVER_MNG("tc_server_management_check_bind() Done checking on topic id %u\n",topic->topic_id);	

	return ERR_OK;
}

int tc_server_management_check_unbind( TOPIC_ENTRY *topic )
{
	DEBUG_MSG_SERVER_MNG("tc_server_management_check_unbind() ...\n");

	int i,j;
	char has_partner;
	NODE_BIND_ENTRY *prod_entry = NULL, *cons_entry = NULL;

	NODE_BIND_ENTRY *producers[MAX_MULTI_NODES]; unsigned int n_prod = 0;
	NODE_BIND_ENTRY *consumers[MAX_MULTI_NODES]; unsigned int n_cons = 0;

	NODE_BIND_ENTRY *rx_node_list[MAX_MULTI_NODES]; unsigned int rx_n_nodes = 0;
	NODE_BIND_ENTRY *tx_node_list[MAX_MULTI_NODES]; unsigned int tx_n_nodes = 0;
	NODE_BIND_ENTRY *err_node_list[MAX_MULTI_NODES];unsigned int n_node_err = 0;

	if ( !init ){
		fprintf(stderr,"tc_server_management_check_unbind() : MODULE ISNT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic );

#if ENABLE_DEBUG_SERVER_MANAGEMENT
	printf("tc_server_management_check_unbind() : DATABASE BEFORE UNBIND\n");
	tc_server_db_topic_print();
#endif

	//Filter producers list and get the entries of those which are bound or requesting a bind
	memset(producers,0,sizeof(producers));
	for ( n_prod = 0, prod_entry = topic->prod_list; prod_entry; prod_entry = prod_entry->next ){
		if ( prod_entry->is_bound ){
			//Producer is bound or requesting a bind -> store its entry
			producers[n_prod] = prod_entry;
			n_prod++;
		}
			
		//Check if we reached max number of nodes
		if ( n_prod >= MAX_MULTI_NODES ){
			fprintf(stderr,"tc_server_management_check_unbind() : MAX NUMBER OF PRODUCER NODES REACHED\n");
			return ERR_NODE_MAX;	
		}
	}	

	//Filter consumers list and get the entries of those which are bound or requesting a bind
	memset(consumers,0,sizeof(consumers));
	for ( n_cons = 0, cons_entry = topic->cons_list; cons_entry; cons_entry = cons_entry->next ){
		if ( cons_entry->is_bound ){
			//Consumer is bound or requesting a bind -> store its entry
			consumers[n_cons] = cons_entry;
			n_cons++;
		}

		//Check if we reached max number of nodes
		if ( n_cons >= MAX_MULTI_NODES ){
			fprintf(stderr,"tc_server_management_check_unbind() : MAX NUMBER OF CONSUMER NODES REACHED\n");
			return ERR_NODE_MAX;	
		}
	}

	//For each bound node check if it still has a valid partner (also bound and not requesting to be unbound)
	//NOTE: When one node is producer and consumer of the same topic, when it produces it produces only for the other consumers (no loopback)

	//Check all producers
	for ( i = 0; i < n_prod ; i++ ){
		//Search for valid consumer partner
		for ( has_partner = j = 0; j < n_cons; j ++){
			//A node which is consumer and producer of same topic is not a valid partnership (no loopback) or a node requesting unbind
			if ( (consumers[j]->node != producers[i]->node) && (!consumers[j]->req_unbind) ){
				has_partner = 1;
				break;
			}	
		}

		if ( !has_partner || producers[i]->req_unbind ){
			//Has no valid partner -- Unbind
			printf("tc_server_management_check_unbind() : going to unbind node %s as tx to topic %d\n", producers[i]->node->address.name_ip,topic->topic_id);
			tx_node_list[tx_n_nodes] = producers[i];
			tx_n_nodes++;
		}
	}

	//Check all consumers
	for ( i = 0; i < n_cons ; i++ ){
		//Search for valid producer partner
		for ( has_partner = j = 0; j < n_prod; j ++){
			//A node which is consumer and producer of same topic is not a valid partnership (no loopback)
			if ( (producers[j]->node != consumers[i]->node) && (!producers[j]->req_unbind) ){
				has_partner = 1;
				break;
			}	
		}

		if ( !has_partner || consumers[i]->req_unbind ){	
			//Has no valid partner -- Unbind
			printf("tc_server_management_check_unbind() : going to unbind node %s as rx to topic %d\n", consumers[i]->node->address.name_ip,topic->topic_id);
			rx_node_list[rx_n_nodes] = consumers[i];
			rx_n_nodes++;
		}
	}

//what to do when some nodes fail to unbind ?

#if ENABLE_DEBUG_SERVER_MANAGEMENT
	printf("\tc_server_management_check_unbind() : tx_node_list: (%d)\n",tx_n_nodes);
	for ( i = 0; i < tx_n_nodes; i++ ){
		printf("tx_node_list[%d] = %d\n",i,tx_node_list[i]->node->node_id);
	}

	printf("tc_server_management_check_unbind() : rx_node_list: (%d)\n",rx_n_nodes);
	for ( i = 0; i < rx_n_nodes; i++ ){
		printf("rx_node_list[%d] = %d\n",i,rx_node_list[i]->node->node_id);
	}
#endif

	//Send unbind request to all the producer nodes
	if ( tx_n_nodes && topic_multi_op_request( tx_node_list, tx_n_nodes, topic, UNBIND_TX, err_node_list, &n_node_err ) ){
		fprintf(stderr,"tc_server_management_check_unbind() : ERROR UNBINDING ONE OR MORE PRODUCER NODES\n");
		return ERR_NODE_BIND;
	}

	//Send unbind request to all the consumer nodes(if any)
	if ( rx_n_nodes && topic_multi_op_request( rx_node_list, rx_n_nodes, topic, UNBIND_RX, err_node_list, &n_node_err ) ){
		fprintf(stderr,"tc_server_management_check_unbind() : ERROR UNBINDING ONE OR MORE CONSUMER NODES\n");
		return ERR_NODE_BIND;
	}

	//Update database and node status
	for ( i = 0; i < rx_n_nodes ; i++ ){
		rx_node_list[i]->is_bound = 0;
		rx_node_list[i]->req_unbind = 0;
	}

	for ( i = 0; i < tx_n_nodes ; i++ ){
		tx_node_list[i]->is_bound = 0;
		tx_node_list[i]->req_unbind = 0;
	}

#if ENABLE_DEBUG_SERVER_MANAGEMENT
	printf("tc_server_management_check_unbind() : DATABASE AFTER UNBIND\n");
	tc_server_db_topic_print();
#endif

	DEBUG_MSG_SERVER_MNG("tc_server_management_check_unbind() Done checking on topic id %u\n",topic->topic_id);	

	return ERR_OK;
}

static int tc_server_management_open_req_sock( void )
{
	DEBUG_MSG_SERVER_MNG("tc_server_management_open_req_sock() ...\n");

	NET_ADDR host,group;

	//Create local server socket
	if ( sock_open( &req_local_sock, LOCAL ) < 0){
		fprintf(stderr,"tc_server_management_open_req_sock() : ERROR CREATING LOCAL SERVER SOCKET\n");
		return ERR_SOCK_CREATE;
	}

	//Set and bind to local host address
	strcpy(host.name_ip, SERVER_MANAGEMENT_REQ_LOCAL_FILE);
	host.port = 0;

	if ( sock_bind( &req_local_sock, &host ) ){
		fprintf(stderr,"tc_server_management_open_req_sock() : ERROR BINDING SOCKET TO LOCAL HOST ADDRESS\n");
		sock_close(&req_local_sock);
		return ERR_SOCK_BIND_HOST;
	}

	//Create remote server socket
	if ( sock_open( &req_remote_sock, REMOTE_UDP_GROUP ) < 0){
		fprintf(stderr,"tc_server_management_open_req_sock() : ERROR CREATING SERVER SOCKET\n");
		sock_close( &req_local_sock );
		return ERR_SOCK_CREATE;
	}

	//Set and bind to remote host address
	strcpy(host.name_ip,server_addr.name_ip);
	host.port = MANAGEMENT_GROUP_PORT;

	if ( sock_bind( &req_remote_sock, &host ) ){
		fprintf(stderr,"tc_server_management_open_req_sock() : ERROR BINDING SOCKET TO REMOTE HOST ADDRESS\n");
		sock_close( &req_local_sock );
		sock_close( &req_remote_sock );
		return ERR_SOCK_BIND_HOST;
	}

	//Set group address
	strcpy(group.name_ip, MANAGEMENT_GROUP_IP);
	group.port = MANAGEMENT_GROUP_PORT;	

	//Join discovery group
	if ( sock_connect_group_tx( &req_remote_sock, &group ) ){
		fprintf(stderr,"tc_server_management_open_req_sock() : ERROR REGISTERING TO MANAGEMENT GROUP\n");
		sock_close ( &req_local_sock );
		sock_close ( &req_remote_sock );
		return ERR_SOCK_BIND_PEER;
	}

	DEBUG_MSG_SERVER_MNG("tc_server_management_open_req_sock() Request sockets created\n");

	return ERR_OK;
}

static int tc_server_management_close_req_sock( void )
{
	DEBUG_MSG_SERVER_MNG("tc_server_management_close_req_sock() ...\n");

	if ( sock_close ( &req_local_sock ) ){
		fprintf(stderr,"tc_server_management_close_req_sock() : ERROR CLOSING LOCAL REQUEST SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	if ( sock_close( &req_remote_sock ) ){
		fprintf(stderr,"tc_server_management_close_req_sock() : ERROR CLOSING REMOTE REQUEST SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	DEBUG_MSG_SERVER_MNG("tc_server_management_close_req_sock() Request sockets destroyed\n");

	return ERR_OK;
}

static int tc_server_management_open_ans_sock( void )
{
	DEBUG_MSG_SERVER_MNG("tc_server_management_open_ans_sock() ...\n");

	NET_ADDR host;

	//Create local server socket
	if ( sock_open( &ans_local_sock, LOCAL ) < 0){
		fprintf(stderr,"tc_server_management_open_ans_sock() : ERROR CREATING LOCAL SERVER SOCKET\n");
		return ERR_SOCK_CREATE;
	}

	//Set and bind to local host address
	strcpy(host.name_ip, SERVER_MANAGEMENT_ANS_LOCAL_FILE);
	host.port = 0;

	if ( sock_bind( &ans_local_sock, &host ) ){
		fprintf(stderr,"tc_server_management_open_ans_sock() : ERROR BINDING SOCKET TO LOCAL HOST ADDRESS\n");
		sock_close(&ans_local_sock);
		return ERR_SOCK_BIND_HOST;
	}

	//Create remote server socket
	if ( sock_open( &ans_remote_sock, REMOTE_UDP ) < 0){
		fprintf(stderr,"tc_server_management_open_ans_sock() : ERROR CREATING SERVER SOCKET\n");
		sock_close( &ans_local_sock );
		return ERR_SOCK_CREATE;
	}

	//Set and bind to remote host address
	strcpy(host.name_ip,server_addr.name_ip);
	host.port = server_addr.port + MANAGEMENT_PORT_OFFSET;

	if ( sock_bind( &ans_remote_sock, &host ) ){
		fprintf(stderr,"tc_server_management_open_ans_sock() : ERROR BINDING SOCKET TO REMOTE HOST ADDRESS\n");
		sock_close( &ans_local_sock );
		sock_close( &ans_remote_sock );
		return ERR_SOCK_BIND_HOST;
	}

	DEBUG_MSG_SERVER_MNG("tc_server_management_open_ans_sock() Reply sockets created\n");

	return ERR_OK;
}

static int tc_server_management_close_ans_sock( void )
{
	DEBUG_MSG_SERVER_MNG("tc_server_management_close_ans_sock() ...\n");

	if ( sock_close ( &ans_local_sock ) ){
		fprintf(stderr,"tc_server_management_close_ans_sock() : ERROR CLOSING LOCAL REPLY SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	if ( sock_close( &ans_remote_sock ) ){
		fprintf(stderr,"tc_server_management_close_ans_sock() : ERROR CLOSING REMOTE REPLY SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	DEBUG_MSG_SERVER_MNG("tc_server_management_close_ans_sock() Reply sockets destroyed\n");

	return ERR_OK;
}

static int topic_multi_op_request( NODE_BIND_ENTRY *node_list[], unsigned int n_nodes, TOPIC_ENTRY *topic, unsigned char op_type, NODE_BIND_ENTRY *ret_err_list[], unsigned int *ret_n_err )
{
	DEBUG_MSG_SERVER_MNG("topic_multi_op_request() ...\n");

	NET_ADDR client;
	NET_MSG request;
	NET_MSG answer;
	struct timeval timeout;
	fd_set fds;
	int highest_fd;

	int i, j;
	unsigned int err_nodes_id[MAX_MULTI_NODES], n_err_nodes;
	unsigned int n_req_nodes;

	if ( !init ){
		fprintf(stderr,"topic_multi_op_request() : MODULE ISNT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	assert( node_list );
	assert( n_nodes );
	assert( topic );
	assert( op_type == BIND_TX || op_type == BIND_RX || op_type == UNBIND_TX || op_type == UNBIND_RX || op_type == DEL_TOPIC || op_type == SET_TOPIC_PROP );
	assert( ret_err_list );
	assert( ret_n_err );

	//Prepare request msg
	memset(&request,0,sizeof(NET_MSG));

	request.type 		= REQ_MSG;
	request.op 		= op_type;
	request.topic_id 	= topic->topic_id;
	request.topic_load 	= topic->topic_load;
	request.topic_addr 	= topic->address;
	request.channel_size 	= topic->channel_size;
	request.channel_period 	= topic->channel_period;
	for ( i = 0; i < n_nodes; i++ ){ request.node_ids[i] = node_list[i]->node->node_id; }
	request.n_nodes		= n_nodes;
	
	//Send message to local nodes
	strcpy(client.name_ip, CLIENT_MANAGEMENT_REQ_LOCAL_FILE);
	client.port = 0;

	tc_network_send_msg( &req_local_sock, &request, &client );
	
	//Send message to remote nodes
	strcpy(client.name_ip, MANAGEMENT_GROUP_IP);
	client.port = MANAGEMENT_GROUP_PORT;

	tc_network_send_msg( &req_remote_sock, &request, &client );

	//Wait for client answers (wait until timeout or received ans from all the requested nodes)
	memset(err_nodes_id,0,sizeof(err_nodes_id));
	n_err_nodes = 0;
	n_req_nodes = n_nodes;

	//Get highest socket fd
	highest_fd = ans_remote_sock.fd;
	if ( ans_local_sock.fd > ans_remote_sock.fd )
		highest_fd = ans_local_sock.fd;

	
	while( n_req_nodes ){
		FD_ZERO(&fds);
		FD_SET(ans_remote_sock.fd, &fds);
		FD_SET(ans_local_sock.fd, &fds);

		timeout.tv_sec = 0;
		timeout.tv_usec = S_REQUESTS_TIMEOUT*1000;

		if ( select(highest_fd+1, &fds, 0, 0, &timeout) <= 0 ) 
			//Timeout waiting for answers (either we received all or several nodes didn't replied)
			break;

		if ( FD_ISSET(ans_local_sock.fd, &fds) ){
			//Received request from a client that is in the same local node as server
			tc_network_get_msg( &ans_local_sock, 0, &answer, NULL );

			//Check if operation was successfull
			if( answer.type != ANS_MSG || answer.error ){
				fprintf(stderr,"topic_multi_op_request() : ERROR ON OPERATION %c BY NODE ID %u ON TOPIC ID %u\n",op_type,answer.node_ids[0],topic->topic_id);
				err_nodes_id[n_err_nodes] = answer.node_ids[0];
				n_err_nodes++;
			}else{
				//Received affirmative reply from one node
				n_req_nodes--;
			}
		}

		if ( FD_ISSET(ans_remote_sock.fd, &fds) ){
			//Received request from a client that is in a remote node
			tc_network_get_msg( &ans_remote_sock, 0, &answer, NULL );

			//Check if operation was successfull
			if( answer.type != ANS_MSG || answer.error ){
				fprintf(stderr,"topic_multi_op_request() : ERROR ON OPERATION %c BY NODE ID %u ON TOPIC ID %u\n",op_type,answer.node_ids[0],topic->topic_id);
				err_nodes_id[n_err_nodes] = answer.node_ids[0];
				n_err_nodes++;
			}else{
				//Received affirmative reply from one node
				n_req_nodes--;
			}		
		}
	}

	//If any node failed to reply or replied negative, return it on the node list
	if ( n_err_nodes ){
		*ret_n_err = 0;
		for ( i = 0; i < n_nodes; i++ ){
			for ( j = 0; j < n_err_nodes; j++ ){
				if( node_list[i]->node->node_id == err_nodes_id[j] ){
					ret_err_list[*ret_n_err] = node_list[i];
					(*ret_n_err)++;
				}
			}
		}

		return -2;
	}

	DEBUG_MSG_SERVER_MNG("topic_multi_op_request() Operation %c on topic id %u by multiple nodes sucessfull\n",op_type,topic->topic_id);

	return ERR_OK;
}
