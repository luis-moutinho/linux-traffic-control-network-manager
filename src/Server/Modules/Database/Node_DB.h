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

/**	@file Node_DB.h
*	@brief Function prototypes and implementation for the node database submodule of the servers database
*
*	This submodule contains the functions to manage and handle the node related database of the servers database.
*	Internal submodule
*  
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef NODEDATABASE_H
#define NODEDATABASE_H

NODE_ENTRY* tc_server_db_node_create( unsigned int node_id )
{
	DEBUG_MSG_NODE_DB("tc_server_db_node_create() NODE_ID %u\n",node_id);

	NODE_ENTRY *db_ptr = NULL;
	NODE_ENTRY *prev_ptr = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_db_node_create() : MODULE NOT RUNNING\n");
		return NULL;
	}

	assert( node_id );

	//Check if entry for this topic already exits
	if ( (db_ptr = tc_server_db_node_search( node_id )) ){
		DEBUG_MSG_NODE_DB("tc_server_db_node_create() : An entry for the node ID %u already exists\n",node_id);
		return db_ptr;
	}

	//Find last element of list
	for( db_ptr = node_db ; db_ptr != NULL ; db_ptr = db_ptr->next ){
		prev_ptr = db_ptr;
	}
	
	//Create new list entry
	if( (db_ptr = (NODE_ENTRY *) malloc(sizeof(NODE_ENTRY))) == NULL ){
		fprintf(stderr,"tc_server_db_node_create() : NOT ENOUGH MEMORY TO REGISTER NEW ENTRY FOR NODE ID %u\n",node_id);
		return NULL;
  	}
	
	//Init entry
	memset(db_ptr,0,sizeof(NODE_ENTRY));

	//First element of list created?
	if ( !prev_ptr )
		node_db = db_ptr;
	else{
		db_ptr->previous = prev_ptr;
		prev_ptr->next = db_ptr;
	}

	DEBUG_MSG_NODE_DB("tc_server_db_node_create() Returning with created entry of Node Id %u\n",node_id);

	return db_ptr;
}

NODE_ENTRY* tc_server_db_node_search( unsigned int node_id )
{
	DEBUG_MSG_NODE_DB("tc_server_db_node_search() NODE_ID %u\n",node_id);

	NODE_ENTRY *db_ptr = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_db_node_search() : MODULE NOT RUNNING\n");
		return NULL;
	}

	assert( node_id );

	//Search node entry
	for( db_ptr = node_db ; db_ptr != NULL ; db_ptr = db_ptr->next ){
		if ( db_ptr->node_id == node_id ){
			DEBUG_MSG_NODE_DB("tc_server_db_node_search() : RETURNING WITH NODE_ID %u ENTRY ADDRESS\n",node_id);
			return db_ptr;
		}
	}
	
	DEBUG_MSG_NODE_DB("tc_server_db_node_search() : ENTRY FOR NODE_ID %u NOT FOUND\n",node_id);

	return NULL;
}

NODE_ENTRY* tc_server_db_node_get_first( void )
{
	DEBUG_MSG_NODE_DB("tc_server_db_node_get_first() ...\n");

	if ( !init ){
		fprintf(stderr,"tc_server_db_node_get_first() : MODULE NOT RUNNING\n");
		return NULL;
	}

	DEBUG_MSG_NODE_DB("tc_server_db_node_get_first() Returning first database entry\n");

	return node_db;
}

int tc_server_db_node_delete( NODE_ENTRY *node )
{
	DEBUG_MSG_NODE_DB("tc_server_db_node_delete() ... \n");

	if ( !init ){
		fprintf(stderr,"tc_server_db_node_delete() : MODULE NOT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	assert( node );

	if ( node->previous )
		(node->previous)->next = node->next;
	else
		node_db = node->next;

	if( node->next )
		(node->next)->previous = node->previous;

	free(node);		

	DEBUG_MSG_NODE_DB("tc_server_db_node_delete() Returning 0\n");

	return ERR_OK;
}

int tc_server_db_node_print( void )
{
	DEBUG_MSG_NODE_DB("tc_server_db_node_print() ...\n");

	NODE_ENTRY *db_ptr = NULL;

	if ( !init ){
		fprintf(stderr,"tc_server_db_node_print() : MODULE NOT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	printf("\n\n Node DB \n\n");

	for( db_ptr = node_db; db_ptr != NULL; db_ptr = db_ptr->next ){
		printf("entry #%p\n",db_ptr);
		printf("node_id %u\n",db_ptr->node_id);
		printf("heartbeat %d\n",db_ptr->heartbeat);
		printf("uplink load %u\n",db_ptr->uplink_load);
		printf("downlink load %u\n",db_ptr->downlink_load);
		
		printf("\nnext #%p\n",db_ptr->next);
		printf("previous #%p\n",db_ptr->previous);
		printf("\n");
	}

	DEBUG_MSG_NODE_DB("tc_server_db_node_print() Returning 0\n");

	return ERR_OK;
}

int tc_server_db_node_print_entry( NODE_ENTRY *entry )
{
	DEBUG_MSG_NODE_DB("tc_server_db_node_print_entry() ...\n");

	if ( !init ){
		fprintf(stderr,"tc_server_db_node_print_entry() : MODULE NOT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	assert( entry );

	printf("\nentry #%p\n",entry);
	printf("node_id %u\n",entry->node_id);
	printf("heartbeat %d\n",entry->heartbeat);
	printf("uplink load %u\n",entry->uplink_load);
	printf("downlink load %u\n",entry->downlink_load);
	
	printf("\nnext #%p\n",entry->next);
	printf("previous #%p\n",entry->previous);

	DEBUG_MSG_NODE_DB("tc_server_db_node_print() Returning 0\n");

	return ERR_OK;
}

#endif
