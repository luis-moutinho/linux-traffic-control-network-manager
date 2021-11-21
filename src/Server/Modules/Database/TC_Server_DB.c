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

/**	@file TC_Server_DB.c
*	@brief Source code of the functions for the server database module
*
*	This file contains the implementation of the functions for the server database
*	module. This module stores and manages topic and node entries in a local database. Internal module
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include "TC_Error_Types.h"
#include "TC_Server_DB.h"
#include "TC_Config.h"

#if ENABLE_DEBUG_SERVER_DB
#define DEBUG_MSG_SERVER_DB(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG_SERVER_DB(...)
#endif

#if ENABLE_DEBUG_SERVER_TOPIC_DB
#define DEBUG_MSG_TOPIC_DB(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG_TOPIC_DB(...)
#endif

#if ENABLE_DEBUG_SERVER_NODE_DB
#define DEBUG_MSG_NODE_DB(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG_NODE_DB(...)
#endif

static NODE_ENTRY *node_db;
static TOPIC_ENTRY *topic_db;

static pthread_mutex_t db_mutex;
static char init = 0;

//Server database calls split in two header files for easier search of functions
#include "Topic_DB.h"
#include "Node_DB.h"

int tc_server_db_lock( void )
{
	DEBUG_MSG_SERVER_DB("tc_server_db_lock() ...\n");

	int ret;

	if ( !init ){
		fprintf(stderr,"tc_server_db_lock() : MODULE NOT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	ret = pthread_mutex_lock(&db_mutex);

	if ( ret == EOWNERDEAD ){
		fprintf(stderr,"tc_server_db_lock() : PREVIOUS HOLDING THREAD TERMINATED WHILE HOLDING MUTEX LUCK");
		pthread_mutex_consistent(&db_mutex);
	}

	if ( ret == EAGAIN ){
		fprintf(stderr,"tc_server_db_lock() : MAX NUMBER RECURSIVE LOCKS EXCEEDED");
		return -1;
	}
		
	if ( ret == EINVAL ){
		fprintf(stderr,"tc_server_db_lock() : CALLING THREAD PRIORITY HIGHER THAN MUTEX PRIORITY/MUTEX NOT INITIALIZED");
		return -2;
	}
	
	if ( ret == ENOTRECOVERABLE ){
		fprintf(stderr,"tc_server_db_lock() : MUTEX IS NOT RECOVERABLE");
		return -3;
	}

	if ( ret == EDEADLK ){
		fprintf(stderr,"tc_server_db_lock() : CURRENT THREAD ALREADY OWNS THE MUTEX");
		return ERR_OK;
	}

	if ( ret == EFAULT ){
		fprintf(stderr,"tc_server_db_lock() : INVALID MUTEX POINTER");
		return -6;
	}

	if ( ret == ENOTRECOVERABLE ){
		fprintf(stderr,"tc_server_db_lock() : MUTEX IS NOT RECOVERABLE");
		return -7;
	}

	DEBUG_MSG_SERVER_DB("tc_server_db_lock() Entered server database\n");

	return ERR_OK;
}

int tc_server_db_unlock( void )
{
	DEBUG_MSG_SERVER_DB("tc_server_db_unlock() ...\n");

	int ret;

	if ( !init ){
		fprintf(stderr,"tc_server_db_unlock() : MODULE NOT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	ret = pthread_mutex_unlock(&db_mutex);

	if ( ret == EAGAIN ){
		fprintf(stderr,"tc_server_db_unlock() : MAX NUMBER RECURSIVE LOCKS EXCEEDED");
		return -1;
	}
		
	if ( ret == EINVAL ){
		fprintf(stderr,"tc_server_db_unlock() : CALLING THREAD PRIORITY HIGHER THAN MUTEX PRIORITY/MUTEX NOT INITIALIZED");
		return -2;
	}

	if ( ret == EPERM ){
		fprintf(stderr,"tc_server_db_unlock() : THREAD DOES NOT OWN THE MUTEX");
		return -3;
	}

	DEBUG_MSG_SERVER_DB("tc_server_db_unlock() Left Server database\n");

	return ERR_OK;
}

int tc_server_db_init( void )
{
	DEBUG_MSG_SERVER_DB("tc_server_db_init() ...\n");

	if ( init ){
		fprintf(stderr,"tc_server_db_init() : MODULE ALREADY RUNNING\n");
		return -1;
	}

	//Initialize stuff
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
	pthread_mutex_init( &db_mutex, &attr );

	topic_db = NULL;
	node_db = NULL;

	init = 1;

	DEBUG_MSG_TOPIC_DB("tc_server_db_init() Returning 0\n");

	return ERR_OK;
}

int tc_server_db_close( void )
{
	DEBUG_MSG_TOPIC_DB("tc_server_db_close() ...\n");

	if ( !init ){
		fprintf(stderr,"tc_server_db_close() : MODULE NOT RUNNING\n");
		return -1;
	}

	//Stop further dabatase access
	init = 0;

	//Wait if someone was inside...
	pthread_mutex_lock(&db_mutex);

	//Delete all topic entries (if any)
	while ( topic_db ){
		if ( tc_server_db_topic_delete(topic_db) < 0 ){
			fprintf(stderr,"tc_server_db_close() : ERROR DELETING TOPIC ENTRY\n");
			pthread_mutex_unlock(&db_mutex);
			init = 1;
			return ERR_TOPIC_DELETE;
		}
	}

	//Delete all node entries (if any)
	while ( node_db ){
		if ( tc_server_db_node_delete(node_db) < 0 ){
			fprintf(stderr,"tc_server_db_close() : ERROR DELETING NODE ENTRY\n");
			pthread_mutex_unlock(&db_mutex);
			init = 1;
			return ERR_NODE_DELETE;
		}
	}

	pthread_mutex_unlock(&db_mutex);
	pthread_mutex_destroy(&db_mutex);

	DEBUG_MSG_TOPIC_DB("tc_server_db_close() Returning 0\n");

	return ERR_OK;
}
