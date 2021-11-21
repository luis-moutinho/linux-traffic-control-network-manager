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

/**	@file TC_Client_DB.c
*	@brief Source code of the functions for the client database module
*
*	This file contains the implementation of the functions for the client database
*	module. This module stores and manages topic entries (with topic information) in a local database. Internal module
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
#include <assert.h>
#include <errno.h>

#include "TC_Client_DB.h"
#include "TC_Config.h"
#include "TC_Error_Types.h"
#include "Sockets.h"

/** 	@def DEBUG_MSG_CLIENT_DB
*	@brief If "ENABLE_DEBUG_CLIENT_DB" is defined debug messages related to this module are printed
*/
#if ENABLE_DEBUG_CLIENT_DB
#define DEBUG_MSG_CLIENT_DB(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG_CLIENT_DB(...)
#endif

static TOPIC_C_ENTRY *topic_db;
static pthread_mutex_t db_mutex;
static char init = 0;

int tc_client_db_init( void )
{
	DEBUG_MSG_CLIENT_DB("tc_client_db_init() ...\n");

	if ( init ){
		fprintf(stderr,"tc_client_db_init() : MODULE ALREADY RUNNING\n");
		return ERR_C_ALREADY_INIT;
	}

	//Initialize stuff
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
	pthread_mutex_init( &db_mutex, &attr );
	topic_db = NULL;

	init = 1;

	DEBUG_MSG_CLIENT_DB("tc_client_db_init() Topic database module initialized\n");

	return ERR_OK;
}

int tc_client_db_close( void )
{
	DEBUG_MSG_CLIENT_DB("tc_client_db_close() ...\n");

	if ( !init ){
		fprintf(stderr,"tc_client_db_close() : MODULE NOT RUNNING\n");
		return ERR_C_NOT_INIT;
	}

	//Stop further dabatase access
	init = 0;

	//Wait if someone was inside...
	pthread_mutex_lock(&db_mutex);

	//Delete all entries (if any)
	while ( topic_db ){

		if ( tc_client_db_topic_delete(topic_db) < 0 ){
			fprintf(stderr,"tc_client_db_close() : ERROR DELETING ENTRY\n");
			pthread_mutex_unlock(&db_mutex);
			init = 1;
			return ERR_TOPIC_DELETE;
		}
	}

	pthread_mutex_unlock(&db_mutex);
	pthread_mutex_destroy(&db_mutex);

	DEBUG_MSG_CLIENT_DB("tc_client_db_close() Topic database module closed\n");

	return ERR_OK;
}

int tc_client_db_lock( void )
{
	DEBUG_MSG_CLIENT_DB("tc_client_db_lock() ...\n");

	int ret;

	if ( !init ){
		fprintf(stderr,"tc_client_db_lock() : MODULE NOT RUNNING\n");
		return ERR_C_NOT_INIT;
	}

	ret = pthread_mutex_lock(&db_mutex);

	if ( ret == EOWNERDEAD ){
		fprintf(stderr,"tc_client_db_lock() : PREVIOUS HOLDING THREAD TERMINATED WHILE HOLDING MUTEX LUCK");
		pthread_mutex_consistent(&db_mutex);
	}

	if ( ret == EAGAIN ){
		fprintf(stderr,"tc_client_db_lock() : MAX NUMBER RECURSIVE LOCKS EXCEEDED");
		return -1;
	}
		
	if ( ret == EINVAL ){
		fprintf(stderr,"tc_client_db_lock() : CALLING THREAD PRIORITY HIGHER THAN MUTEX PRIORITY/MUTEX NOT INITIALIZED");
		return -2;
	}
	
	if ( ret == ENOTRECOVERABLE ){
		fprintf(stderr,"tc_client_db_lock() : MUTEX IS NOT RECOVERABLE");
		return -3;
	}

	if ( ret == EDEADLK ){
		fprintf(stderr,"tc_client_db_lock() : CURRENT THREAD ALREADY OWNS THE MUTEX");
		return ERR_OK;
	}

	if ( ret == EFAULT ){
		fprintf(stderr,"tc_client_db_lock() : INVALID MUTEX POINTER");
		return -6;
	}

	if ( ret == ENOTRECOVERABLE ){
		fprintf(stderr,"tc_client_db_lock() : MUTEX IS NOT RECOVERABLE");
		return -7;
	}

	DEBUG_MSG_CLIENT_DB("tc_client_db_lock() Entered topic database\n");

	return ERR_OK;
}

int tc_client_db_unlock( void )
{
	DEBUG_MSG_CLIENT_DB("tc_client_db_unlock() ...\n");

	int ret;

	if ( !init ){
		fprintf(stderr,"tc_client_db_unlock() : MODULE NOT RUNNING\n");
		return ERR_C_NOT_INIT;
	}

	ret = pthread_mutex_unlock(&db_mutex);

	if ( ret == EAGAIN ){
		fprintf(stderr,"tc_client_db_unlock() : MAX NUMBER RECURSIVE LOCKS EXCEEDED");
		return -1;
	}
		
	if ( ret == EINVAL ){
		fprintf(stderr,"tc_client_db_unlock() : CALLING THREAD PRIORITY HIGHER THAN MUTEX PRIORITY/MUTEX NOT INITIALIZED");
		return -2;
	}

	if ( ret == EPERM ){
		fprintf(stderr,"tc_client_db_unlock() : THREAD DOES NOT OWN THE MUTEX");
		return -3;
	}

	DEBUG_MSG_CLIENT_DB("tc_client_db_unlock() Left topic database\n");

	return ERR_OK;
}

TOPIC_C_ENTRY* tc_client_db_topic_search( unsigned int topic_id )
{
	DEBUG_MSG_CLIENT_DB("tc_client_db_topic_search () Topic Id %u\n",topic_id);

	TOPIC_C_ENTRY *db_ptr = NULL;

	assert( topic_id );

	//Search channel entry
	for( db_ptr = topic_db ; db_ptr != NULL ; db_ptr = db_ptr->next ){
		if ( db_ptr->topic_id == topic_id ){
			DEBUG_MSG_CLIENT_DB("tc_client_db_topic_search() : Returning with Topic Id %u entry address\n",topic_id);
			return db_ptr;
		}
	}
	
	DEBUG_MSG_CLIENT_DB("tc_client_db_topic_search() : ENTRY FOR TOPIC ID %u NOT FOUND\n",topic_id);

	return NULL;
}

int tc_client_db_topic_delete( TOPIC_C_ENTRY *topic )
{
	DEBUG_MSG_CLIENT_DB("tc_client_db_topic_delete() ... \n");

	assert( topic );

	//Failsafe
	if ( topic->topic_sock.fd > 0 ){
		topic->is_consumer = 0;
		topic->is_producer = 0;
		topic->is_closing = 1;

		if ( topic->unblock_rx_sock.fd > 0 ){
			sock_send( &topic->unblock_rx_sock, &topic->unblock_rx_sock.host, "0", 5);
			sock_close( &topic->unblock_rx_sock );
		}
		sock_close( &topic->topic_sock );
	}

	if ( topic->previous )
		(topic->previous)->next = topic->next;
	else
		topic_db = topic->next;

	if( topic->next )
		(topic->next)->previous = topic->previous;

	pthread_mutex_destroy( &(topic->topic_rx_lock) );	
	pthread_mutex_destroy( &(topic->topic_tx_lock) );

	free(topic);		

	DEBUG_MSG_CLIENT_DB("tc_client_db_topic_delete() Returning 0\n");

	return ERR_OK;
}

TOPIC_C_ENTRY* tc_client_db_topic_create( unsigned int topic_id )
{
	DEBUG_MSG_CLIENT_DB("tc_client_db_topic_create() Topic Id %u\n",topic_id);

	TOPIC_C_ENTRY *db_ptr = NULL;
	TOPIC_C_ENTRY *prev_ptr = NULL;

	assert( topic_id );

	//Check if entry for this topic already exits
	if ( (db_ptr = tc_client_db_topic_search( topic_id )) ){
		DEBUG_MSG_CLIENT_DB("tc_client_db_topic_create() : An entry for the topic ID %d already exists\n",topic_id);
		return db_ptr;
	}

	//There is no entry for this topic -> Find last element of list
	for( db_ptr = topic_db ; db_ptr != NULL ; db_ptr = db_ptr->next ){
		prev_ptr = db_ptr;
	}
	
	//Create new list entry
	if( (db_ptr = (TOPIC_C_ENTRY *) malloc(sizeof(TOPIC_C_ENTRY))) == NULL ){
		fprintf(stderr,"tc_client_db_topic_create() : NOT ENOUGH MEMORY TO REGISTER NEW ENTRY FOR TOPIC ID %u\n",topic_id);
		return NULL;
  	}
	
	//Init entry
	memset(db_ptr,0,sizeof(TOPIC_C_ENTRY));

	//Init mutexes
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);

	pthread_mutex_init( &(db_ptr->topic_rx_lock), &attr );
	pthread_mutex_init( &(db_ptr->topic_tx_lock), &attr );

	//First element of list created?
	if ( !prev_ptr )
		topic_db = db_ptr;
	else{
		db_ptr->previous = prev_ptr;
		prev_ptr->next = db_ptr;
	}

	DEBUG_MSG_CLIENT_DB("tc_client_db_topic_create() Returning with created entry of Topic Id %u\n",topic_id);

	return db_ptr;
}

int tc_client_db_topic_print( void )
{
	DEBUG_MSG_CLIENT_DB("tc_client_db_topic_print() ...\n");

	TOPIC_C_ENTRY *db_ptr;

	printf("\n\n Topic DB \n\n");

	for( db_ptr = topic_db; db_ptr != NULL; db_ptr = db_ptr->next ){
		printf("entry #%p\n",db_ptr);
		printf("topic_id %u\n",db_ptr->topic_id);
		printf("size %u\n",db_ptr->channel_size);
		printf("period %u\n",db_ptr->channel_period);
		printf("next #%p\n",db_ptr->next);
		printf("previous #%p\n",db_ptr->previous);
		printf("\n");
	}

	DEBUG_MSG_CLIENT_DB("tc_client_db_topic_print() Returning 0\n");

	return ERR_OK;
}

int tc_client_lock_topic_tx( TOPIC_C_ENTRY *topic, unsigned int timeout )
{
	DEBUG_MSG_CLIENT_DB("tc_client_lock_topic_tx() ...\n");

	int ret;
	unsigned int tries;

	assert( topic );

	if ( timeout > 0 ){
		for ( tries = 0; tries < timeout && (ret = pthread_mutex_trylock( &(topic->topic_tx_lock) )); tries++ ){
			usleep(1000);
			if ( ret < 0 )
				printf("lock_topic_tx got error\n");

			if ( ret == EOWNERDEAD ){
				fprintf(stderr,"tc_client_lock_topic_tx() : PREVIOUS HOLDING THREAD TERMINATED WHILE HOLDING MUTEX LUCK");
				pthread_mutex_consistent(&topic->topic_tx_lock);
				return ERR_OK;
			}
		}
	
		if ( tries == timeout )
			return -2;
	}else{
		ret = pthread_mutex_lock( &(topic->topic_tx_lock) );
		if ( ret < 0 )
			printf("unlock_topic_tx got error\n");

		if ( ret == EOWNERDEAD ){
				fprintf(stderr,"tc_client_lock_topic_tx() : PREVIOUS HOLDING THREAD TERMINATED WHILE HOLDING MUTEX LUCK");
				pthread_mutex_consistent(&topic->topic_tx_lock);
		}
	}

	DEBUG_MSG_CLIENT_DB("tc_client_lock_topic_tx() Got lock on topic id %u\n",topic->topic_id);

	return ERR_OK;
}

int tc_client_unlock_topic_tx( TOPIC_C_ENTRY *topic )
{
	DEBUG_MSG_CLIENT_DB("tc_client_unlock_topic_tx() ...\n");

	int ret;

	assert( topic );

	ret = pthread_mutex_unlock( &(topic->topic_tx_lock) );

	if ( ret == EAGAIN ){
		fprintf(stderr,"tc_client_unlock_topic_tx() : MAX NUMBER RECURSIVE LOCKS EXCEEDED");
		return -1;
	}
		
	if ( ret == EINVAL ){
		fprintf(stderr,"tc_client_unlock_topic_tx() : CALLING THREAD PRIORITY HIGHER THAN MUTEX PRIORITY/MUTEX NOT INITIALIZED");
		return -2;
	}

	if ( ret == EPERM ){
		fprintf(stderr,"tc_client_unlock_topic_tx() : THREAD DOES NOT OWN THE MUTEX");
		return -3;
	}

	DEBUG_MSG_CLIENT_DB("tc_client_lock_topic_tx() Released lock on topic id %u\n",topic->topic_id);

	return ERR_OK;
}

int tc_client_lock_topic_rx( TOPIC_C_ENTRY *topic, unsigned int timeout )
{
	DEBUG_MSG_CLIENT_DB("tc_client_lock_topic_rx() ...\n");

	int ret;

	unsigned int tries;

	assert( topic );

	if ( timeout > 0 ){
		for ( tries = 0; tries < timeout && (ret = pthread_mutex_trylock( &(topic->topic_rx_lock) )); tries++ ){
			usleep(1000);
			if ( ret == EOWNERDEAD ){
				fprintf(stderr,"tc_client_lock_topic_rx() : PREVIOUS HOLDING THREAD TERMINATED WHILE HOLDING MUTEX LUCK");
				pthread_mutex_consistent(&topic->topic_rx_lock);
				return ERR_OK;
			}
		}
	
		if ( tries == timeout )
			return -2;
	}else{
		ret = pthread_mutex_lock( &(topic->topic_rx_lock) );
		if ( ret == EOWNERDEAD ){
			fprintf(stderr,"tc_client_lock_topic_rx() : PREVIOUS HOLDING THREAD TERMINATED WHILE HOLDING MUTEX LUCK");
			pthread_mutex_consistent(&topic->topic_rx_lock);
		}
	}

	DEBUG_MSG_CLIENT_DB("tc_client_lock_topic_rx() Got lock on topic id %u\n",topic->topic_id);

	return ERR_OK;
}

int tc_client_unlock_topic_rx( TOPIC_C_ENTRY *topic )
{
	DEBUG_MSG_CLIENT_DB("tc_client_unlock_topic_rx() ...\n");

	int ret;

	assert( topic );

	ret = pthread_mutex_unlock( &(topic->topic_rx_lock) );

	if ( ret == EAGAIN ){
		fprintf(stderr,"tc_client_unlock_topic_rx() : MAX NUMBER RECURSIVE LOCKS EXCEEDED");
		return -1;
	}
		
	if ( ret == EINVAL ){
		fprintf(stderr,"tc_client_unlock_topic_rx() : CALLING THREAD PRIORITY HIGHER THAN MUTEX PRIORITY/MUTEX NOT INITIALIZED");
		return -2;
	}

	if ( ret == EPERM ){
		fprintf(stderr,"tc_client_unlock_topic_rx() : THREAD DOES NOT OWN THE MUTEX");
		return -3;
	}

	DEBUG_MSG_CLIENT_DB("tc_client_lock_topic_rx() Released lock on topic id %u\n",topic->topic_id);

	return ERR_OK;
}
