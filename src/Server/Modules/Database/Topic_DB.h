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

/**	@file Topic_DB.h
*	@brief Function prototypes and implementation for the topic database submodule of the servers database
*
*	This submodule contains the functions to manage and handle the topic related database of the servers database.
*	Internal submodule
*  
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef TOPICDATABASE_H
#define TOPICDATABASE_H

TOPIC_ENTRY* tc_server_db_topic_get_first( void )
{
	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_get_first() ...\n");

	if ( !init ){
		fprintf(stderr,"tc_server_db_topic_get_first() : MODULE NOT RUNNING\n");
		return NULL;
	}
	
	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_get_first() Returning first database entry\n");

	return topic_db;
}

TOPIC_ENTRY* tc_server_db_topic_search( unsigned int topic_id )
{
	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_search() Topic Id %u ...\n",topic_id);

	TOPIC_ENTRY *topic = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_db_topic_search() : MODULE NOT RUNNING\n");
		return NULL;
	}

	assert( topic_id );

	//Search node entry
	for( topic = topic_db ; topic ; topic = topic->next ){
		if ( topic->topic_id == topic_id ){
			DEBUG_MSG_TOPIC_DB("tc_server_db_topic_search() : RETURNING WITH TOPIC ID %u ENTRY ADDRESS\n",topic_id);
			return topic;
		}
	}
	
	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_search() : ENTRY FOR TOPIC ID %u NOT FOUND\n",topic_id);

	return NULL;
}

TOPIC_ENTRY* tc_server_db_topic_create( unsigned int topic_id )
{
	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_create() Topic Id %u ...\n",topic_id);

	TOPIC_ENTRY *db_ptr = NULL;
	TOPIC_ENTRY *prev_ptr = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_db_topic_create() : MODULE NOT RUNNING\n");
		return NULL;
	}

	assert( topic_id );

	//Check if entry for this topic already exits
	if ( (db_ptr = tc_server_db_topic_search( topic_id )) ){
		DEBUG_MSG_TOPIC_DB("tc_server_db_topic_create() : An entry for the topic ID %u already exists\n",topic_id);
		return db_ptr;
	}

	//Find last element of list
	for( db_ptr = topic_db ; db_ptr ; db_ptr = db_ptr->next ){
		prev_ptr = db_ptr;
	}
	
	//Create new list entry
	if( (db_ptr = (TOPIC_ENTRY *) malloc(sizeof(TOPIC_ENTRY))) == NULL ){
		fprintf(stderr,"tc_server_db_topic_create() : NOT ENOUGH MEMORY TO REGISTER NEW ENTRY FOR TOPIC ID %u\n",topic_id);
		return NULL;
  	}
	
	//Init entry
	memset(db_ptr,0,sizeof(TOPIC_ENTRY));

	//First element of list created?
	if ( !prev_ptr )
		topic_db = db_ptr;
	else{
		db_ptr->previous = prev_ptr;
		prev_ptr->next = db_ptr;
	}

	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_create() Returning with created entry of Topic Id %u\n",topic_id);

	return db_ptr;
}

int tc_server_db_topic_delete( TOPIC_ENTRY *topic )
{
	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_delete() ... \n");

	assert( topic );

	if ( topic->previous )
		(topic->previous)->next = topic->next;
	else
		topic_db = topic->next;

	if( topic->next )
		(topic->next)->previous = topic->previous;

	free(topic);		

	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_delete() Returning 0\n");

	return ERR_OK;
}

NODE_ENTRY* tc_server_db_topic_find_prod_node( TOPIC_ENTRY *topic, unsigned int node_id )
{
	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_find_prod_node() Node Id %u ...\n",node_id);

	NODE_BIND_ENTRY *entry;

	if ( !init ){
		fprintf(stderr,"tc_server_db_topic_find_prod_node() : MODULE NOT RUNNING\n");
		return NULL;
	}

	assert( topic );
	assert( node_id );

	//Search for node in producers list
	for ( entry = topic->prod_list ; entry ; entry = entry->next ){
		if ( (entry->node)->node_id == node_id ){
			DEBUG_MSG_TOPIC_DB("tc_server_db_topic_find_prod_node() : RETURNING WITH NODE ID %u ENTRY ADDRESS\n",node_id);
			return entry->node;
		}	
	}

	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_find_prod_node() : ENTRY FOR NODE ID %u NOT FOUND\n",node_id);

	return NULL;
}

NODE_ENTRY* tc_server_db_topic_find_cons_node( TOPIC_ENTRY *topic, unsigned int node_id )
{
	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_find_cons_node() Node Id %u ...\n",node_id);

	NODE_BIND_ENTRY *entry;

	if ( !init ){
		fprintf(stderr,"tc_server_db_topic_find_cons_node() : MODULE NOT RUNNING\n");
		return NULL;
	}

	assert( topic );
	assert( node_id );

	//Search for node in consumers list
	for( entry = topic->cons_list; entry ; entry = entry->next ){
		if ( (entry->node)->node_id == node_id ){
			DEBUG_MSG_TOPIC_DB("tc_server_db_topic_find_cons_node() : RETURNING WITH NODE ID %u ENTRY ADDRESS\n",node_id);
			return entry->node;
		}	
	}

	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_find_cons_node() : ENTRY FOR NODE ID %u NOT FOUND\n",node_id);

	return NULL;
}

int tc_server_db_topic_add_prod_node( TOPIC_ENTRY *topic, NODE_ENTRY *node )
{
	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_add_prod_node() ...\n");
	
	NODE_BIND_ENTRY *entry = NULL, *prev_entry = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_db_topic_add_prod_node() : MODULE NOT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic );
	assert( node );

	//Check if node already registered
	for( entry = topic->prod_list; entry ; prev_entry = entry , entry = entry->next ){
		if ( entry->node == node ){
			DEBUG_MSG_TOPIC_DB("tc_server_db_topic_add_prod_node() : NODE ID %u ALREADY REGISTERED AS PRODUCER OF TOPIC ID %u\n",node->node_id,topic->topic_id);
			return ERR_OK;
		}
	}

	//Add new producer entry
	if( !(entry = (NODE_BIND_ENTRY *) malloc( sizeof(NODE_BIND_ENTRY) )) ){
		fprintf(stderr,"tc_server_db_topic_add_prod_node() : NOT ENOUGH MEMORY TO CREATE NEW ENTRY FOR NODE ID %u\n",node->node_id);
		return ERR_MEM_MALLOC;
  	}

	//Fill entry
	entry->node = node;
	entry->is_bound = 0;
	entry->req_bind = 0;
	entry->next = entry->previous = NULL;

	//First element of list created?
	if ( !prev_entry )
		topic->prod_list = entry;
	else{
		entry->previous = prev_entry;
		prev_entry->next = entry;
	}

	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_add_prod_node() : ADDED NODE ID %u AS PRODUCER OF TOPIC ID %u\n",node->node_id,topic->topic_id);

	return ERR_OK;
}

int tc_server_db_topic_rm_prod_node( TOPIC_ENTRY *topic, NODE_ENTRY *node )
{
	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_rm_prod_node() ...\n");
	
	NODE_BIND_ENTRY *entry;

	if ( !init ){
		fprintf(stderr,"tc_server_db_topic_rm_prod_node() : MODULE NOT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic );
	assert( node );

	//Find and remove node from list
	for( entry = topic->prod_list; entry ; entry = entry->next ){
		if ( entry->node == node ){

			//Remove entry
			if ( entry->previous )
				//Last or middle list entry
				(entry->previous)->next = entry->next;
			else
				//First list entry
				topic->prod_list = entry->next;

			if( entry->next )
				(entry->next)->previous = entry->previous;

			free(entry);
		
			DEBUG_MSG_TOPIC_DB("tc_server_db_topic_rm_prod_node() : Removed Node Id %u from Topic Id %u producer list\n",node->node_id,topic->topic_id);
			return ERR_OK;
		}	
	}

	fprintf(stderr,"tc_server_db_topic_rm_prod_node() : NODE ID %u ENTRY NOT FOUND IN TOPIC ID %u PRODUCER LIST\n",node->node_id,topic->topic_id);

	return ERR_NODE_NOT_REG_TX;
}

int tc_server_db_topic_add_cons_node( TOPIC_ENTRY *topic, NODE_ENTRY *node )
{
	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_add_cons_node() ...\n");
	
	NODE_BIND_ENTRY *entry = NULL, *prev_entry = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_db_topic_add_cons_node() : MODULE NOT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic );

	assert( node );

	//Check if node already registered
	for( entry = topic->cons_list; entry ; prev_entry = entry , entry = entry->next ){
		if ( entry->node == node ){
			fprintf(stderr,"tc_server_db_topic_add_cons_node() : NODE ID %u ALREADY REGISTERED AS CONSUMER OF TOPIC ID %u\n",node->node_id,topic->topic_id);
			return ERR_OK;
		}
	}

	//Add new producer entry
	if( !(entry = (NODE_BIND_ENTRY *) malloc( sizeof(NODE_BIND_ENTRY) )) ){
		fprintf(stderr,"tc_server_db_topic_add_cons_node() : NOT ENOUGH MEMORY TO CREATE NEW ENTRY FOR NODE ID %u\n",node->node_id);
		return ERR_MEM_MALLOC;
  	}

	//Fill entry
	entry->node = node;
	entry->is_bound = 0;
	entry->req_bind = 0;
	entry->next = entry->previous = NULL;

	//First element of list created?
	if ( !prev_entry )
		topic->cons_list = entry;
	else{
		entry->previous = prev_entry;
		prev_entry->next = entry;
	}

	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_add_cons_node() : ADDED NODE ID %u AS PRODUCER OF TOPIC ID %u\n",node->node_id,topic->topic_id);

	return ERR_OK;
}

int tc_server_db_topic_rm_cons_node( TOPIC_ENTRY *topic, NODE_ENTRY *node )
{
	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_rm_cons_node() ...\n");
	
	NODE_BIND_ENTRY *entry;

	if ( !init ){
		fprintf(stderr,"tc_server_db_topic_rm_cons_node() : MODULE NOT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic );
	assert( node );

	//Find and remove node from list
	for( entry = topic->cons_list; entry ; entry = entry->next ){
		if ( entry->node == node ){

			//Remove entry
			if ( entry->previous )
				//Last or middle list entry
				(entry->previous)->next = entry->next;
			else
				//First list entry
				topic->cons_list = entry->next;

			if( entry->next )
				(entry->next)->previous = entry->previous;

			free(entry);

			DEBUG_MSG_TOPIC_DB("tc_server_db_topic_rm_cons_node() : Removed Node Id %u from Topic Id %u consumer list\n",node->node_id,topic->topic_id);
			return ERR_OK;
		}	
	}

	fprintf(stderr,"tc_server_db_topic_rm_cons_node() : NODE ID %u ENTRY NOT FOUND IN TOPIC ID %u CONSUMER LIST\n",node->node_id,topic->topic_id);

	return ERR_NODE_NOT_REG_RX;
}

int tc_server_db_topic_number_cons_nodes( TOPIC_ENTRY *topic )
{
	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_number_cons_nodes() ...\n");
	
	NODE_BIND_ENTRY *entry;
	int count = 0;

	if ( !init ){
		fprintf(stderr,"tc_server_db_topic_number_cons_nodes() : MODULE NOT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic );

	//Search for nodes in consumers list
	for( entry = topic->cons_list; entry ; entry = entry->next ){
		count++;
	}

	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_number_cons_nodes() : Topic Id %u has %u consumer nodes\n",topic->topic_id,count);

	return count;
}

int tc_server_db_topic_number_prod_nodes( TOPIC_ENTRY *topic )
{
	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_number_prod_nodes() ...\n");
	
	NODE_BIND_ENTRY *entry;
	int count = 0;

	if ( !init ){
		fprintf(stderr,"tc_server_db_topic_number_prod_nodes() : MODULE NOT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	assert( topic );

	//Search for nodes in producers list
	for( entry = topic->prod_list; entry ; entry = entry->next ){
		count++;
	}

	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_number_prod_nodes() : Topic Id %u has %u producer nodes\n",topic->topic_id,count);

	return count;
}

int tc_server_db_topic_print( void )
{
	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_print() ...\n");
	
	TOPIC_ENTRY *db_ptr = NULL;
	NODE_BIND_ENTRY *entry = NULL;

	printf("\n\n Topic DB \n\n");

	for( db_ptr = topic_db; db_ptr != NULL; db_ptr = db_ptr->next ){
		printf("entry #%p\n",db_ptr);
		printf("topic_id %u\n",db_ptr->topic_id);
		printf("topic size %u\n",db_ptr->channel_size);
		printf("topic period %u\n",db_ptr->channel_period);
		printf("Producer Nodes\n");
		for( entry = db_ptr->prod_list; entry ; entry = entry->next ){
			printf("%p r %d b %d\t",entry->node,entry->req_bind,entry->is_bound);
			fflush(stdout);
		}
		printf("\nConsumer Nodes\n");
		for( entry = db_ptr->cons_list; entry ; entry = entry->next ){
			printf("%p r %d b %d\t",entry->node,entry->req_bind,entry->is_bound);
			fflush(stdout);
		}
		printf("\nnext #%p\n",db_ptr->next);
		printf("previous #%p\n",db_ptr->previous);
		printf("\n");
	}

	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_print() Returning 0\n");

	return ERR_OK;
}

int tc_server_db_topic_print_entry( TOPIC_ENTRY *entry )
{
	DEBUG_MSG_TOPIC_DB("tc_server_db_topic_print_entry() ...\n");

	NODE_BIND_ENTRY *aux;

	assert( entry );

	printf("\nentry #%p\n",entry);
	printf("topic_id %u\n",entry->topic_id);
	printf("topic size %u\n",entry->channel_size);
	printf("topic period %u\n",entry->channel_period);
	printf("Producer Nodes\n");
	for( aux = entry->prod_list; aux ; aux = aux->next ){
		printf("%p r %d b %d\t",aux->node,aux->req_bind,aux->is_bound);
		fflush(stdout);
	}
	printf("\nConsumer Nodes\n");
	for( aux = entry->cons_list; aux ; aux = aux->next ){
		printf("%p r %d b %d\t",aux->node,aux->req_bind,aux->is_bound);
		fflush(stdout);
	}
	printf("\nnext #%p\n",entry->next);
	printf("previous #%p\n",entry->previous);

	DEBUG_MSG_TOPIC_DB("node_db_print() Returning 0\n");

	return ERR_OK;
}

#endif
