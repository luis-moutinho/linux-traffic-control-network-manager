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

/**	@file TC_Server_AC.c
*	@brief Source code of the functions for the server admission control module
*
*	This file contains the implementation of the functions for the server admission control module. 
*	This module receives the requests from the client and accepts or refuses them. Uses other internal modules
*	to carry out the necessary operations in order to fulfill the accepted requests.
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

#include "TC_Data_Types.h"
#include "TC_Error_Types.h"
#include "TC_Config.h"

#include "TC_Server_AC.h"
#include "TC_Server_DB.h"
#include "TC_Server_Management.h"
#include "TC_Server_Notifications.h"

static char init = 0;

//Find a better way to create topic addresses and assign node IDs (address pool maybe ?)
//To generate unique ip for each group we split the port number (I.E. Port = XXYZZ -> 239.0XX.00Y.0ZZ)
static unsigned int topic_port = 10000;
static unsigned int node_id_pool = 10000;

/**	@def DEBUG_MSG_SERVER_AC
*	@brief If "ENABLE_DEBUG_SERVER_AC" is defined debug messages related to this module are printed
*/
#if ENABLE_DEBUG_SERVER_AC
#define DEBUG_MSG_SERVER_AC(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG_SERVER_AC(...)
#endif

static int tc_server_ac_check_bw( TOPIC_ENTRY *topic, NODE_ENTRY *cons_node, NODE_ENTRY *prod_node, unsigned int req_load );

int tc_server_ac_init( void )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_init() ...\n");

	if ( init ){
		fprintf(stderr,"tc_server_ac_init() : MODULE IS ALREADY RUNNING\n");
		return ERR_S_ALREADY_INIT;
	}

	init = 1;
	
	DEBUG_MSG_SERVER_AC("tc_server_ac_init() Admission control module is now running\n");

	return ERR_OK;
}

int tc_server_ac_close( void )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_close() ...\n");

	if ( !init ){
		fprintf(stderr,"tc_server_ac_close() : MODULE ISNT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	init = 0;

	DEBUG_MSG_SERVER_AC("tc_server_ac_close() Admission control module is now closed\n");

	return ERR_OK;
}

int tc_server_ac_add_node( unsigned int node_id, NET_ADDR *node_address, unsigned int *ret_node_id )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_add_node() Node Id %u ...\n",node_id);

	NODE_ENTRY *node = NULL;
	unsigned int req_node_id = node_id;

	if ( !init ){
		fprintf(stderr,"tc_server_ac_add_node() : MODULE ISNT RUNNING\n");
		return ERR_S_NOT_INIT;
	}
	
	assert( node_address ); 
	assert( ret_node_id ); 

	if ( req_node_id ){
		//User wants a specific node ID
		//Check if node with this ID is registered
		if ( (node = tc_server_db_node_search( req_node_id )) ){
			//Already registered with same address (shouldnt happen)
			if( !strcmp(node->address.name_ip, node_address->name_ip) && (node->address.port == node_address->port) ){
				fprintf(stderr,"tc_server_ac_add_node() : Node Id %u with address %s:%u already registered\n",req_node_id,node->address.name_ip,node->address.port);
				return ERR_OK;
			}
			//Already registered with different address
			fprintf(stderr,"tc_server_ac_add_node() : Node ID %u already registered with different address (%s:%u)\n",req_node_id,node->address.name_ip,node->address.port); 
			return ERR_NODE_DIFF_ADDR;	
		}

	}else{
		//User wants server to assign a node ID
		req_node_id = node_id_pool;
		node_id_pool++;

		//Check if node with this ID is registered
		while ( (node = tc_server_db_node_search( req_node_id )) ){
			req_node_id = node_id_pool++;	
		}
	}
	
	if ( !(node = tc_server_db_node_create( req_node_id )) ){
		fprintf(stderr,"tc_server_ac_add_node() : ERROR ADDING NEW ENTRY FOR NODE ID %u\n",req_node_id);
		return ERR_REG_NODE;
	}

	//Fill new node entry
	node->node_id = req_node_id;
	strcpy(node->address.name_ip,node_address->name_ip);
	node->address.port = node_address->port;
	node->heartbeat = HEARBEAT_COUNT;

	*ret_node_id = req_node_id;

	//Send notification
	tc_server_notifications_send_node_event( EVENT_NODE_PLUG, node );

	DEBUG_MSG_SERVER_AC("tc_server_ac_add_node() Node Id %u with address %s:%u registered\n",req_node_id,node->address.name_ip,node->address.port);

	return ERR_OK;
}

int tc_server_ac_rm_node( unsigned int node_id )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_rm_node() Node Id %u ...\n",node_id);

	NODE_ENTRY *node = NULL;
	NODE_ENTRY aux;

	if ( !init ){
		fprintf(stderr,"tc_server_ac_rm_node() : MODULE ISNT RUNNING\n");
		return ERR_S_NOT_INIT;
	}
	
	assert( node_id );

	//Check if node is registered
	node = tc_server_db_node_search( node_id );

	if ( !node ){
		fprintf(stderr,"tc_server_ac_rm_node() : NODE ID %u NOT REGISTERED\n",node_id);
		return ERR_NODE_NOT_REG;
	}

	//Copy node entry for node event call
	aux = *node;

	//Call management module to handle node removal
	if ( tc_server_management_rm_node( node ) ){
		fprintf(stderr,"tc_server_ac_rm_node() : ERROR REMOVING NODE ID %u\n",node_id);
		return ERR_UNREG_NODE;
	}

	//Send node removal notification
	tc_server_notifications_send_node_event( EVENT_NODE_UNPLUG, &aux );

	DEBUG_MSG_SERVER_AC("tc_server_ac_rm_node() Node Id %u removed\n",node_id);

	return ERR_OK;
}

int tc_server_ac_add_topic( unsigned int topic_id, unsigned int channel_size, unsigned int channel_period )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_add_topic() Topic Id %u ...\n",topic_id);

	TOPIC_ENTRY *topic = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_ac_add_topic() : MODULE IS NOT INITIALIZED\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic_id );
	assert( channel_size );
	assert( channel_period );

	//Check if topic is registered
	topic = tc_server_db_topic_search( topic_id );

	if ( topic ){
		//Already registered with same properties
		if( topic->channel_size == channel_size && topic->channel_period == channel_period ){
			fprintf(stderr,"tc_server_ac_add_topic() : Topic Id %u Size %u Period %u already registered\n",topic_id,channel_size,channel_period);
			return ERR_OK;
		}
		//Already registered with different properties
		fprintf(stderr,"tc_server_ac_add_topic() : TOPIC ID %u EXISTS WITH SIZE %u AND PERIOD %u\n",topic_id,topic->channel_size,topic->channel_period); 
		fprintf(stderr,"tc_server_ac_add_topic() : REQUESTED SIZE %u PERIOD %u. USE TOPIC CHANGE PROPERTIES CALL INSTEAD!\n",channel_size,channel_period);
		return ERR_TOPIC_DIFF_PROP;	
	}
	
	if ( !(topic = tc_server_db_topic_create( topic_id )) ){
		fprintf(stderr,"tc_server_ac_add_topic() : ERROR ADDING NEW ENTRY FOR TOPIC ID %u\n",topic_id);
		return ERR_TOPIC_CREATE;
	}

	//Fill new topic entry
	topic->topic_id = topic_id;
	//To generate unique ip for each group we split the port number (I.E. Port = XXYZZ -> 239.0XX.00Y.0ZZ)
	sprintf(topic->address.name_ip,"239.1%d.10%d.1%d", topic_port/1000, (topic_port/100)%10, topic_port%100);
	topic->address.port = topic_port;
	topic->channel_size = channel_size;
	topic->channel_period = channel_period;
	topic->topic_load = (topic->channel_size * 8000 / topic->channel_period) * RESERV_SLACK_MULTIPLIER;

	topic_port++;

	DEBUG_MSG_SERVER_AC("tc_server_ac_add_topic() Topic Id %u Size %u Period %u registered\n",topic_id,topic->channel_size,topic->channel_period);

	return ERR_OK;
}

int tc_server_ac_set_topic_prop( unsigned int topic_id, unsigned int channel_size, unsigned int channel_period )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_set_topic_prop() Topic Id %u ...\n",topic_id);

	TOPIC_ENTRY updated_topic, *topic = NULL;
	NODE_BIND_ENTRY *cons_entry = NULL, *prod_entry = NULL;

	int n_prod,ret;
	unsigned int final_load,current_load;
	int delta_load;

	if ( !init ){
		fprintf(stderr,"tc_server_ac_set_topic_prop() : MODULE IS NOT INITIALIZED\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic_id );
	assert( channel_size );
	assert( channel_period );

	//Check if topic is registered
	topic = tc_server_db_topic_search( topic_id );

	if ( !topic ){
		fprintf(stderr,"tc_server_ac_set_topic_prop(): TOPIC ID %u NOT REGISTERED\n",topic_id);
		return ERR_TOPIC_NOT_REG;
	}else{
		//Registered with same properties
		if( topic->channel_size == channel_size && topic->channel_period == channel_period ){
			fprintf(stderr,"tc_server_ac_set_topic_prop() : Topic Id %u Size %u Period %u already registered\n",topic_id,channel_size,channel_period);
			return ERR_OK;
		}
	}

	//Calculate the amount of load that will be negotiated (can be more or less than actual load)
	final_load = (channel_size * 8000 / channel_period) * RESERV_SLACK_MULTIPLIER;
	current_load = topic->topic_load;
	delta_load = final_load - current_load;

	DEBUG_MSG_SERVER_AC("tc_server_ac_set_topic_prop() : final_load %u current_load %u delta_load %d\n",final_load,current_load,delta_load);

	//If we are requesting more load check if every node has enough bandwidth for the required changes
	if ( final_load > current_load ){
		if ( (delta_load > 0) && (ret = tc_server_ac_check_bw( topic, NULL, NULL, (unsigned int)delta_load )) ){
			fprintf(stderr,"tc_server_ac_set_topic_prop() : NOT ENOUGH BANDWIDTH ON SOME OR ALL NODES FOR TOPIC ID %u CHANGES\n",topic_id);
			return ret;
		}	
	}

	//Call management module to update nodes reservations and local database with the new topic properties
	updated_topic = *topic;
	updated_topic.topic_load = final_load;
	updated_topic.channel_size = channel_size;
	updated_topic.channel_period = channel_period;

	if ( tc_server_management_set_topic( &updated_topic ) ){
		fprintf(stderr,"tc_server_ac_set_topic_prop() : ERROR UPDATING TOPIC ID %u PROPERTIES ON NODES\n",topic_id);
		return ERR_TOPIC_UPDATE;
	}

	//Update producers bandwidth
	for ( prod_entry = topic->prod_list; prod_entry ; prod_entry = prod_entry->next  )
		prod_entry->node->uplink_load = prod_entry->node->uplink_load + delta_load;

	//NOTE : When one node is producer and consumer of the same topic, when it produces it produces only for the other consumers (no loopback)
	//Update consumers bandwidth
	for ( cons_entry = topic->cons_list; cons_entry ; cons_entry = cons_entry->next  ){

		//Get number of valid producers for this consumer
		for( n_prod = 0, prod_entry = topic->prod_list; prod_entry; prod_entry = prod_entry->next ){
			if ( cons_entry->node != prod_entry->node )
				n_prod++;
		}
		cons_entry->node->downlink_load = cons_entry->node->downlink_load + (delta_load * n_prod);
	}

	//Update topic entry
	*topic = updated_topic;

	DEBUG_MSG_SERVER_AC("tc_server_ac_set_topic_prop() Topic Id %u New size %u New Period %u\n",topic_id,topic->channel_size,topic->channel_period);

	return ERR_OK;
}

int tc_server_ac_rm_topic( unsigned int topic_id )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_rm_topic() Topic Id %u ...\n",topic_id);

	int n_prod, n_prod_aux;
	TOPIC_ENTRY *topic = NULL;
	NODE_BIND_ENTRY *prod_entry = NULL, *cons_entry = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_ac_rm_topic() : MODULE IS NOT INITIALIZED\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic_id );

	//Check if topic is registered
	topic = tc_server_db_topic_search( topic_id );

	if ( !topic ){
		fprintf(stderr,"tc_server_ac_rm_topic() : TOPIC ID %u NOT REGISTERED\n",topic_id);
		return ERR_TOPIC_NOT_REG;
	}

	//Call management module to handle the topic removal in the nodes
	if ( tc_server_management_rm_topic( topic ) ){
		fprintf(stderr,"tc_server_ac_rm_topic() : ERROR REMOVING TOPIC ID %u ON ALL NODES\n",topic_id);
		return ERR_TOPIC_DELETE;
	} 

	//Update database
	//Get current number of producers for this topic
	n_prod = tc_server_db_topic_number_prod_nodes( topic );

	//Update consumers
	for ( cons_entry = topic->cons_list; cons_entry ; cons_entry = cons_entry->next  ){
		//Update node load ( for consumers the load is proporcional to the number of producer nodes )
		//NOTE1: When one node is producer and consumer of the same topic, when it produces it produces only for the other consumers (no loopback)
		//NOTE2: It can happen that this node was registered as consumer while there was no producers and thus the current load is 0!
		//In this case the load after the update would be < 0

		//Check if this consumer is also producer of the same topic
		for ( prod_entry = topic->prod_list, n_prod_aux = n_prod; prod_entry; prod_entry = prod_entry->next ){
			if ( cons_entry->node == prod_entry->node )
				//Also producer of same topic -- Ignore self load
				n_prod_aux--;
		}
		if ( cons_entry->node->downlink_load )
			cons_entry->node->downlink_load = cons_entry->node->downlink_load - topic->topic_load*n_prod_aux;
 
		//Remove node from topic list
		tc_server_db_topic_rm_cons_node( topic, cons_entry->node );
	}

	//Update producers
	for ( prod_entry = topic->prod_list; prod_entry ; prod_entry = prod_entry->next  ){
		//Update node load
		prod_entry->node->uplink_load = prod_entry->node->uplink_load - topic->topic_load;

		//Remove node from topic list
		tc_server_db_topic_rm_prod_node( topic, prod_entry->node );
	}

	//Destroy topic entry
	if ( tc_server_db_topic_delete( topic ) ){
		fprintf(stderr,"tc_server_ac_rm_topic() : ERROR DESTROYING TOPIC ID %u ENTRY\n",topic_id);
		return -5;
	}

	DEBUG_MSG_SERVER_AC("tc_server_ac_rm_topic() Topic Id %u removed\n",topic_id);

	return ERR_OK;
}

int tc_server_ac_get_topic_prop( unsigned int topic_id, unsigned int *ret_load, unsigned int *ret_size, unsigned int *ret_period, NET_ADDR *ret_topic_addr )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_get_topic_prop() Topic Id %u ...\n",topic_id);

	TOPIC_ENTRY *topic = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_ac_get_topic_prop() : MODULE IS NOT INITIALIZED\n");
		return ERR_S_NOT_INIT;
	}
	
	assert( topic_id );

	//Check if topic is registered
	topic = tc_server_db_topic_search( topic_id );

	if ( !topic ){
		fprintf(stderr,"tc_server_ac_get_topic_prop(): TOPIC ID %u NOT REGISTERED\n",topic_id);
		return ERR_TOPIC_NOT_REG;
	}

	if ( ret_load )
		*ret_load = topic->topic_load;

	if ( ret_size )
		*ret_size = topic->channel_size;

	if ( ret_period )
		*ret_period = topic->channel_period;

	if ( ret_topic_addr ){
		strcpy( ret_topic_addr->name_ip, topic->address.name_ip);
		ret_topic_addr->port = topic->address.port;
	}

	DEBUG_MSG_SERVER_AC("tc_server_ac_get_topic_prop() Got Topic Id %u properties\n",topic_id);

	return ERR_OK;
}

int tc_server_ac_add_prod( unsigned int topic_id, unsigned int node_id )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_add_prod() Topic Id %u ...\n",topic_id);

	int ret;
	NODE_ENTRY *node = NULL; 
	TOPIC_ENTRY *topic = NULL;
	NODE_BIND_ENTRY *cons_entry = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_ac_add_prod() : MODULE IS NOT INITIALIZED\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic_id );
	assert( node_id );

	//Get topic entry
	if ( !(topic = tc_server_db_topic_search( topic_id )) ){
		fprintf(stderr,"tc_server_ac_add_prod() : TOPIC ID %u NOT REGISTERED\n",topic_id);
		return ERR_TOPIC_NOT_REG;
	}

	//Get node entry
	if ( !(node = tc_server_db_node_search( node_id )) ){
		fprintf(stderr,"tc_server_ac_add_prod() : NODE ID %u NOT REGISTERED\n",node_id);
		return ERR_NODE_NOT_REG;
	}

	//Check if node is already a producer of topic	
	if ( tc_server_db_topic_find_prod_node( topic, node_id ) ){
		fprintf(stderr,"tc_server_ac_add_prod() : NODE ID %u ALREADY PRODUCER OF TOPIC ID %d\n",node_id,topic_id);
		return ERR_OK;
	}

	DEBUG_MSG_SERVER_AC("tc_server_ac_add_prod() Node Id %u Uplink load %u [bps] Requesting load %u [bps]\n",node->node_id,node->uplink_load,topic->topic_load);

	//Check if all nodes of this topic have enough bandwidth
	if ( (topic->topic_load > 0) && (ret = tc_server_ac_check_bw( topic, node, NULL, topic->topic_load )) ){
		fprintf(stderr,"tc_server_ac_add_prod() : NOT ENOUGH BANDWIDTH ON SOME OR ALL NODES FOR TOPIC ID %u CHANGES\n",topic_id);
		return ret;
	}	

	//Reserv resources in producer node
	if ( tc_server_management_reserv_req( node, topic, TC_RESERV, topic->topic_load ) ){
		fprintf(stderr,"tc_server_ac_add_prod() : ERROR RESERVING BANDWIDTH FOR TOPIC ID %u ON NODE ID %u\n",topic->topic_id,node->node_id);
		return ERR_NODE_PROD_RESERV;
	}

	//Add producer node to topic list
	if ( tc_server_db_topic_add_prod_node( topic, node ) ){
		fprintf(stderr,"tc_server_ac_add_prod() : ERROR REGISTERING NODE ID %u AS PRODUCER OF TOPIC ID %u\n",node_id,topic_id);
		tc_server_management_reserv_req( node, topic, TC_FREE, topic->topic_load );
		return ERR_NODE_PROD_REG;
	}	
			
	//Update nodes load
	node->uplink_load = node->uplink_load + topic->topic_load;
	for ( cons_entry = topic->cons_list; cons_entry ; cons_entry = cons_entry->next  ){

		//If we are also consumer of this topic -- ignore load (no loopback)		
		if ( cons_entry->node != node )
			cons_entry->node->downlink_load =  cons_entry->node->downlink_load + topic->topic_load;
	}

	DEBUG_MSG_SERVER_AC("tc_server_ac_add_prod() Added Node Id %u as producer of Topic Id %u\n",node_id,topic_id);
	DEBUG_MSG_SERVER_AC("tc_server_ac_add_prod() Node Id %u Uplink load %u [bps] Downlink load %u [bps]\n",node->node_id,node->uplink_load,node->downlink_load);

	return ERR_OK;
}

int tc_server_ac_rm_prod( unsigned int topic_id, unsigned int node_id )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_rm_prod() Topic Id %u ...\n",topic_id);

	NODE_ENTRY *node = NULL; 
	TOPIC_ENTRY *topic = NULL;
	NODE_BIND_ENTRY *cons_entry = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_ac_rm_prod() : MODULE IS NOT INITIALIZED\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic_id );
	assert( node_id );

	//Get topic entry
	if ( !(topic = tc_server_db_topic_search( topic_id )) ){
		fprintf(stderr,"tc_server_ac_rm_prod() : TOPIC ID %u NOT REGISTERED\n",topic_id);
		return ERR_TOPIC_NOT_REG;
	}

	//Get node entry
	if ( !(node = tc_server_db_node_search( node_id )) ){
		fprintf(stderr,"tc_server_ac_rm_prod() : NODE ID %u NOT REGISTERED\n",node_id);
		return ERR_NODE_NOT_REG;
	}

	//Check if node is a producer of topic	
	if ( !tc_server_db_topic_find_prod_node( topic, node_id ) ){
		DEBUG_MSG_SERVER_AC("tc_server_ac_rm_prod() : Node id %u is not registered as producer of topic id %u\n",node_id,topic_id);
		return ERR_OK;
	}

	//Free resources reservation in the node
	if ( tc_server_management_reserv_req( node, topic, TC_FREE, topic->topic_load ) ){
		fprintf(stderr,"tc_server_ac_rm_prod() : ERROR FREEING BANDWIDTH FOR TOPIC ID %u ON NODE ID %u\n",topic->topic_id,node->node_id);
		return ERR_NODE_PROD_F_RESERV;
	}

	//Remove node from producer list
	if ( tc_server_db_topic_rm_prod_node( topic, node ) ){
		fprintf(stderr,"tc_server_ac_rm_prod() : ERROR UNREGISTERING NODE ID %u AS PRODDUCER OF TOPIC ID %u\n",node_id,topic_id);
		tc_server_management_reserv_req( node, topic, TC_RESERV, topic->topic_load );
		return ERR_NODE_PROD_UNREG;
	}	

	//Update producer node load
	node->uplink_load = node->uplink_load - topic->topic_load;

	//Update the bandwidth of all consumer nodes of this topic
	//NOTE1: When one node is producer and consumer of the same topic, when it produces it produces only for the other consumers (no loopback)
	for ( cons_entry = topic->cons_list; cons_entry ; cons_entry = cons_entry->next  ){
		if ( cons_entry->node != node ){
			cons_entry->node->downlink_load = cons_entry->node->downlink_load - topic->topic_load;
		}
	}

	DEBUG_MSG_SERVER_AC("tc_server_ac_rm_prod() Removed Node Id %u as producer of Topic Id %u\n",node_id,topic_id);

	return ERR_OK;
}

int tc_server_ac_add_cons( unsigned int topic_id, unsigned int node_id )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_add_cons() Topic Id %u ...\n",topic_id);

	int ret,n_prod = 0;
	NODE_ENTRY *node = NULL; 
	TOPIC_ENTRY *topic = NULL;
	unsigned int req_load = 0;
	
	if ( !init ){
		fprintf(stderr,"tc_server_ac_add_cons() : MODULE IS NOT INITIALIZED\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic_id );	
	assert( node_id );

	//Get topic entry
	if ( !(topic = tc_server_db_topic_search( topic_id )) ){
		fprintf(stderr,"tc_server_ac_add_cons() : TOPIC ID %u NOT REGISTERED\n",topic_id);
		return ERR_TOPIC_NOT_REG;
	}

	//Get node entry
	if ( !(node = tc_server_db_node_search( node_id )) ){
		fprintf(stderr,"tc_server_ac_add_cons() : NODE ID %u NOT REGISTERED\n",node_id);
		return ERR_NODE_NOT_REG;
	}

	//Check if node is already a consumer of topic
	if ( tc_server_db_topic_find_cons_node( topic, node_id ) ){
		fprintf(stderr,"tc_server_ac_add_cons() : NODE ID %u ALREADY CONSUMER OF TOPIC ID %u\n",node_id,topic_id);
		return ERR_OK;
	}

	//Calculate requested bandwidth for consumer node
	//Check how many producers are registered for this topic
	n_prod = tc_server_db_topic_number_prod_nodes( topic );

	//NOTE : When one node is producer and consumer of the same topic, when it produces it produces only for the other consumers (no loopback)
	if ( tc_server_db_topic_find_prod_node( topic, node_id ) ){
		//Producer for the same topic -> ignore self load
		n_prod--;				
	}
	req_load = n_prod * topic->topic_load; 

	DEBUG_MSG_SERVER_AC("tc_server_ac_add_cons() Node Id %u Downlink load %u [bps] Requesting load %u [bps]\n",node->node_id,node->downlink_load,req_load);

	//Check if all nodes have enough bandwidth
	if ( (req_load > 0) && (ret = tc_server_ac_check_bw( topic, NULL, node, req_load )) ){
		fprintf(stderr,"tc_server_ac_add_cons() : NOT ENOUGH BANDWIDTH ON SOME OR ALL NODES FOR TOPIC ID %u CHANGES\n",topic_id);
		return ret;
	}	

	//Enough bandwidth to add consumer -> Add consumer to list
	if ( tc_server_db_topic_add_cons_node( topic, node ) ){
		fprintf(stderr,"tc_server_ac_add_cons() : ERROR REGISTERING NODE ID %u AS CONSUMER OF TOPIC ID %u\n",node_id,topic_id);
		return ERR_NODE_CONS_REG;
	}	

	//Update node load
	node->downlink_load = node->downlink_load + req_load;

	DEBUG_MSG_SERVER_AC("tc_server_ac_add_cons() Added Node Id %u as consumer of Topic Id %u\n",node_id,topic_id);
	DEBUG_MSG_SERVER_AC("tc_server_ac_add_cons() Node Id %u Uplink load %u [bps] Downlink load %u [bps]\n",node->node_id,node->uplink_load,node->downlink_load);

	return ERR_OK;
}

int tc_server_ac_rm_cons( unsigned int topic_id, unsigned int node_id )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_rm_cons() Topic Id %u ...\n",topic_id);

	int n_prod;
	NODE_ENTRY *node = NULL; 
	TOPIC_ENTRY *topic = NULL;
	unsigned int req_load = 0;
	NODE_BIND_ENTRY *prod_entry = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_ac_rm_cons() : MODULE ISNT RUNNING\n");
		return ERR_S_NOT_INIT;
	}
	
	assert( topic_id );
	assert( node_id );

	//Get topic entry
	if ( !(topic = tc_server_db_topic_search( topic_id )) ){
		fprintf(stderr,"tc_server_ac_rm_cons() : TOPIC ID %u NOT REGISTERED\n",topic_id);
		return ERR_TOPIC_NOT_REG;
	}

	//Get node entry
	if ( !(node = tc_server_db_node_search( node_id )) ){
		fprintf(stderr,"tc_server_ac_rm_cons() : NODE ID %u NOT REGISTERED\n",node_id);
		return ERR_NODE_NOT_REG;
	}

	//Check if node is a consumer of topic	
	if ( !tc_server_db_topic_find_cons_node( topic, node_id ) ){
		DEBUG_MSG_SERVER_AC("tc_server_ac_rm_cons() : Node id %u is not registered as consumer of topic id %u\n",node_id,topic_id);
		return ERR_OK;
	}

	//Remove node from consumers list
	if ( tc_server_db_topic_rm_cons_node( topic, node ) ){
		fprintf(stderr,"tc_server_ac_rm_cons() : ERROR UNREGISTERING NODE ID %u AS CONSUMER OF TOPIC ID %u\n",node_id,topic_id);
		return ERR_NODE_CONS_UNREG;
	}	

	//Calculate node bandwidth for this topic

	//Check valid producers number
	//NOTE: When one node is producer and consumer of the same topic, when it produces it produces only for the other consumers (no loopback)
	for( n_prod = 0, prod_entry = topic->prod_list; prod_entry; prod_entry = prod_entry->next ){
		if ( prod_entry->node != node )
			n_prod++;
	}
	req_load = n_prod * topic->topic_load;

	//Update node load
	node->downlink_load = node->downlink_load - req_load;

	DEBUG_MSG_SERVER_AC("tc_server_ac_rm_cons() Removed Node Id %u as consumer of Topic Id %u\n",node_id,topic_id);

	return ERR_OK;
}

int tc_server_ac_bind_tx( unsigned int topic_id, unsigned int node_id )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_bind_tx() Topic Id %u ...\n",topic_id);

	NODE_ENTRY *node = NULL; 
	TOPIC_ENTRY *topic = NULL;
	NODE_BIND_ENTRY *prod_entry = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_ac_bind_tx() : MODULE IS NOT INITIALIZED\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic_id );
	assert( node_id );

	//Get topic entry
	if ( !(topic = tc_server_db_topic_search( topic_id )) ){
		fprintf(stderr,"tc_server_ac_bind_tx() : TOPIC ID %u NOT REGISTERED\n",topic_id);
		return ERR_TOPIC_NOT_REG;
	}

	//Get node entry
	if ( !(node = tc_server_db_node_search( node_id )) ){
		fprintf(stderr,"tc_server_ac_bind_tx() : NODE ID %u NOT REGISTERED\n",node_id);
		return ERR_NODE_NOT_REG;
	}

	//Check if node is registered as producer of topic	
	if ( !tc_server_db_topic_find_prod_node( topic, node_id ) ){
		fprintf(stderr,"tc_server_ac_bind_tx() : NODE ID %u NOT REGISTERED AS PRODUCER OF TOPIC ID %u\n",node_id,topic_id);
		return ERR_NODE_NOT_REG_TX;
	}

	//Check if node is already bound -- If it isn't, signal management module that the node is requesting a bind
	for ( prod_entry = topic->prod_list; prod_entry; prod_entry = prod_entry->next  ){
		if ( prod_entry->node == node ){
			if ( prod_entry->is_bound ){
				//Node is already bound
				fprintf(stderr,"tc_server_ac_bind_tx() : Node id %u is already bound to topic id %u as producer\n",node_id,topic_id);
				return ERR_OK;
			} 
			prod_entry->req_bind = 1;
			break;
		}
	}

	DEBUG_MSG_SERVER_AC("tc_server_ac_bind_tx() Bind request to topic id %u from node id %u accepted\n",topic_id,node_id);

	return ERR_OK;
}

int tc_server_ac_unbind_tx( unsigned int topic_id, unsigned int node_id )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_unbind_tx() Topic Id %u ...\n",topic_id);

	NODE_ENTRY *node = NULL; 
	TOPIC_ENTRY *topic = NULL;
	NODE_BIND_ENTRY *prod_entry = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_ac_unbind_tx() : MODULE IS NOT INITIALIZED\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic_id );
	assert( node_id );

	//Get topic entry
	if ( !(topic = tc_server_db_topic_search( topic_id )) ){
		fprintf(stderr,"tc_server_ac_unbind_tx() : TOPIC ID %u NOT REGISTERED\n",topic_id);
		return ERR_TOPIC_NOT_REG;
	}

	//Get node entry
	if ( !(node = tc_server_db_node_search( node_id )) ){
		fprintf(stderr,"tc_server_ac_unbind_tx() : NODE ID %u NOT REGISTERED\n",node_id);
		return ERR_NODE_NOT_REG;
	}

	//Check if node is registered as producer of topic	
	if ( !tc_server_db_topic_find_prod_node( topic, node_id ) ){
		fprintf(stderr,"tc_server_ac_unbind_tx() : NODE ID %u NOT REGISTERED AS PRODUCER OF TOPIC ID %u\n",node_id,topic_id);
		return ERR_NODE_NOT_REG_TX;
	}

	//Check if node is bound
	for ( prod_entry = topic->prod_list; prod_entry ; prod_entry = prod_entry->next  ){
		if ( prod_entry->node == node ){
			if ( !prod_entry->is_bound ){
				//Node is not bound
				fprintf(stderr,"tc_server_ac_unbind_tx() : Node id %u is already unbound from topic id %u as producer\n",node_id,topic_id);
				return ERR_OK;
			} 
			prod_entry->req_unbind = 1;
			break;
		}
	}

	DEBUG_MSG_SERVER_AC("tc_server_ac_unbind_tx() Unbind request to topic id %u from node id %u accepted\n",topic_id,node_id);

	return ERR_OK;
}

int tc_server_ac_bind_rx( unsigned int topic_id, unsigned int node_id )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_bind_rx() Topic Id %u ...\n",topic_id);

	NODE_ENTRY *node = NULL; 
	TOPIC_ENTRY *topic = NULL;
	NODE_BIND_ENTRY *cons_entry = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_ac_bind_rx() : MODULE IS NOT INITIALIZED\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic_id );
	assert( node_id );

	//Get topic entry
	if ( !(topic = tc_server_db_topic_search( topic_id )) ){
		fprintf(stderr,"tc_server_ac_bind_rx() : TOPIC ID %u NOT REGISTERED\n",topic_id);
		return ERR_TOPIC_NOT_REG;
	}

	//Get node entry
	if ( !(node = tc_server_db_node_search( node_id )) ){
		fprintf(stderr,"tc_server_ac_bind_rx() : NODE ID %u NOT REGISTERED\n",node_id);
		return ERR_NODE_NOT_REG;
	}

	//Check if node is registered as consumer of topic	
	if ( !tc_server_db_topic_find_cons_node( topic, node_id ) ){
		fprintf(stderr,"tc_server_ac_bind_rx() : NODE ID %u NOT REGISTERED AS CONSUMER OF TOPIC ID %u\n",node_id,topic_id);
		return ERR_NODE_NOT_REG_RX;
	}

	//Check if node is already bound -- If it isn't, signal management module that the node is requesting a bind
	for ( cons_entry = topic->cons_list; cons_entry ; cons_entry = cons_entry->next  ){
		if ( cons_entry->node == node ){
			if ( cons_entry->is_bound ){
				//Node is already bound
				fprintf(stderr,"tc_server_ac_bind_rx() : NODE ID %u IS ALREADY BOUND TO TOPIC ID %u AS CONSUMER\n",node_id,topic_id);
				return ERR_OK;
			} 
			cons_entry->req_bind = 1;
			break;
		}
	}

	DEBUG_MSG_SERVER_AC("tc_server_ac_bind_rx() Bind request to topic id %u from node id %u accepted\n",topic_id,node_id);

	return ERR_OK;
}

int tc_server_ac_unbind_rx( unsigned int topic_id, unsigned int node_id )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_unbind_rx() Topic Id %u ...\n",topic_id);

	NODE_ENTRY *node = NULL; 
	TOPIC_ENTRY *topic = NULL;
	NODE_BIND_ENTRY *cons_entry = NULL;
	
	if ( !init ){
		fprintf(stderr,"tc_server_ac_unbind_rx() : MODULE IS NOT INITIALIZED\n");
		return ERR_S_NOT_INIT;
	}
	
	assert( topic_id );
	assert( node_id );

	//Get topic entry
	if ( !(topic = tc_server_db_topic_search( topic_id )) ){
		fprintf(stderr,"tc_server_ac_unbind_rx() : TOPIC ID %u NOT REGISTERED\n",topic_id);
		return ERR_TOPIC_NOT_REG;
	}

	//Get node entry
	if ( !(node = tc_server_db_node_search( node_id )) ){
		fprintf(stderr,"tc_server_ac_unbind_rx() : NODE ID %u NOT REGISTERED\n",node_id);
		return ERR_NODE_NOT_REG;
	}

	//Check if node is registered as consumer of topic	
	if ( !tc_server_db_topic_find_cons_node( topic, node_id ) ){
		fprintf(stderr,"tc_server_ac_unbind_rx() : NODE ID %u NOT REGISTERED AS CONSUMER OF TOPIC ID %u\n",node_id,topic_id);
		return ERR_NODE_NOT_REG_RX;
	}

	//Check if node is bound
	for ( cons_entry = topic->cons_list; cons_entry ; cons_entry = cons_entry->next  ){
		if ( cons_entry->node == node ){
			if ( !cons_entry->is_bound ){
				//Node is not bound
				fprintf(stderr,"tc_server_ac_unbind_rx() : Node id %u is already unbound from topic id %u as consumer\n",node_id,topic_id);
				return ERR_OK;
			}
			cons_entry->req_unbind = 1;
			break;
		}
	}

	DEBUG_MSG_SERVER_AC("tc_server_ac_unbind_rx() Unbind request to topic id %u from node id %u accepted\n",topic_id,node_id);

	return ERR_OK;
}

int tc_server_ac_check_topic_bind( unsigned int topic_id )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_check_topic_bind() Topic Id %u ...\n",topic_id);

	TOPIC_ENTRY *topic = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_ac_check_topic_bind() : MODULE IS NOT INITIALIZED\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic_id );

	//Get topic entry
	if ( !(topic = tc_server_db_topic_search( topic_id )) ){
		fprintf(stderr,"tc_server_ac_check_topic_bind() : TOPIC ID %u NOT REGISTERED\n",topic_id);
		return ERR_TOPIC_NOT_REG;
	}

	//Call management module to check bind requests for this topic
	if ( tc_server_management_check_bind( topic ) ){
		fprintf(stderr,"tc_server_ac_check_topic_bind() : ERROR IN MANAGEMENT MODULE (TOPIC ID %u)\n",topic_id);
		return ERR_NODE_BIND;
	}	

	DEBUG_MSG_SERVER_AC("tc_server_ac_check_topic_bind() Checked possible binds on topic id %u\n",topic_id);

	return ERR_OK;
}

int tc_server_ac_check_topic_unbind( unsigned int topic_id )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_check_topic_unbind() Topic Id %u ...\n",topic_id);

	TOPIC_ENTRY *topic = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_ac_check_topic_unbind() : MODULE IS NOT INITIALIZED\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic_id );

	//Get topic entry
	if ( !(topic = tc_server_db_topic_search( topic_id )) ){
		fprintf(stderr,"tc_server_ac_check_topic_unbind() : TOPIC ID %u NOT REGISTERED\n",topic_id);
		return ERR_TOPIC_NOT_REG;
	}

	//Call management module to check unbind requests for this topic
	if ( tc_server_management_check_unbind( topic ) ){
		fprintf(stderr,"tc_server_ac_check_topic_unbind() : ERROR IN MANAGEMENT MODULE (TOPIC ID %u)\n",topic_id);
		return ERR_NODE_UNBIND;
	}	

	DEBUG_MSG_SERVER_AC("tc_server_ac_check_topic_unbind() Checked possible binds on topic id %u\n",topic_id);

	return ERR_OK;
}

static int tc_server_ac_check_bw( TOPIC_ENTRY *topic, NODE_ENTRY *cons_node, NODE_ENTRY *prod_node, unsigned int req_load )
{
	DEBUG_MSG_SERVER_AC("tc_server_ac_check_bw() ...\n");

	int n_prod = 0;
	NODE_BIND_ENTRY *cons_entry = NULL, *prod_entry = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_ac_check_bw() : MODULE IS NOT INITIALIZED\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic );
	assert( req_load > 0 );	
	assert( !(cons_node && prod_node) );//Can't add one producer and consumer in the same call! One must be NULL

	if ( !cons_node && !prod_node ){
		//Request is due to topic properties changes

		//Check all producers bandwidth
		for ( prod_entry = topic->prod_list; prod_entry; prod_entry = prod_entry->next ){

			if ( prod_entry->node->uplink_load + req_load > MAX_USABLE_BW ){
				fprintf(stderr,"tc_server_ac_check_bw() : NOT ENOUGH BANDWIDTH FOR TOPIC ID %u ON PROD NODE ID %u\n",topic->topic_id,prod_entry->node->node_id);
				return ERR_NODE_PROD_BW;
			}
		}

		//Check all consumers bandwidth
		//NOTE : When one node is producer and consumer of the same topic, when it produces it produces only for the other consumers (no loopback)
		for ( cons_entry = topic->cons_list; cons_entry; cons_entry = cons_entry->next ){

			//Get number of valid producers for this consumer
			for( n_prod = 0, prod_entry = topic->prod_list; prod_entry; prod_entry = prod_entry->next ){
				if ( cons_entry->node != prod_entry->node )
					n_prod++;
			}
	
			if ( cons_entry->node->downlink_load + (req_load * n_prod) > MAX_USABLE_BW ){
				fprintf(stderr,"tc_server_ac_check_bw() : NOT ENOUGH BANDWIDTH FOR TOPIC ID %u ON CONS NODE ID %u\n",topic->topic_id,cons_entry->node->node_id);
				return ERR_NODE_CONS_BW;
			}
		}

		DEBUG_MSG_SERVER_AC("tc_server_ac_check_bw() There is enough bandwidth for Topid ID %u properties changes\n",topic->topic_id);
		return ERR_OK;
	}

	if ( cons_node ){
		//Request is due to the registration of a consumer node

		if ( cons_node->downlink_load + req_load > MAX_USABLE_BW ){
			fprintf(stderr,"tc_server_ac_check_bw() : NOT ENOUGH DOWNLINK BANDWIDTH ON NODE ID %u FOR TOPIC ID %u\n",cons_node->node_id,topic->topic_id);
			return ERR_NODE_CONS_BW;
		}

		//No need to check bandwidth on producers because we use multicast

		DEBUG_MSG_SERVER_AC("tc_server_ac_check_bw() There is enough bandwidth to add a consumer of Topid ID %u\n",topic->topic_id);
		return ERR_OK;
	}

	if ( prod_node ){
		//Request is due to the registration of a producer node
		if ( prod_node->uplink_load + req_load > MAX_USABLE_BW ){
			fprintf(stderr,"tc_server_ac_check_bw() : NOT ENOUGH UPLINK BANDWIDTH ON NODE ID %u FOR TOPIC ID %u\n",prod_node->node_id,topic->topic_id);
			return ERR_NODE_PROD_BW;
		}
	
		//Check if all consumers of this topic have enough downlink bandwidth
		//NOTE : When one node is producer and consumer of the same topic, when it produces it produces only for the other consumers (no loopback)
		for ( cons_entry = topic->cons_list; cons_entry; cons_entry = cons_entry->next ){

			//Check only for consumer nodes other than ourselves
			if ( cons_entry->node == prod_node ) continue;

			if ( cons_entry->node->downlink_load + req_load > MAX_USABLE_BW ){
				DEBUG_MSG_SERVER_AC("tc_server_ac_check_bw() : NOT ENOUGH DOWNLINK BANDWIDTH ON NODE ID %u\n",cons_entry->node->node_id);
				return ERR_NODE_CONS_BW;
			} 
		}

		DEBUG_MSG_SERVER_AC("tc_server_ac_check_bw() There is enough bandwidth to add a producer of Topid ID %u\n",topic->topic_id);
		return ERR_OK;
	}

	return ERR_OK;
}
