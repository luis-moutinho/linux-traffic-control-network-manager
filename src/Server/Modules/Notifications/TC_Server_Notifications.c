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

/**	@file TC_Server_Notifications.c
*	@brief Source code of the functions for the server notifications module
*
*	This file contains the implementation of the functions for the server notifications module. 
*	This module contains the means to send notification messages to the client notifications module when an event occurs (I.E node registered in the network).
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

#include "TC_Server_DB.h"
#include "TC_Server_Notifications.h"

#include "Sockets.h"
#include "TC_Data_Types.h"
#include "TC_Utils.h"
#include "TC_Config.h"
#include "TC_Error_Types.h"

/**	@def DEBUG_MSG_SERVER_NOTIFICATIONS
*	@brief If "ENABLE_DEBUG_SERVER_NOTIFICATIONS" is defined debug messages related to this module are printed
*/
#if ENABLE_DEBUG_SERVER_NOTIFICATIONS
#define DEBUG_MSG_SERVER_NOTIFICATIONS(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG_SERVER_NOTIFICATIONS(...)
#endif

static char init = 0;

static SOCK_ENTITY notific_local_sock,notific_remote_sock;

int tc_server_notifications_init( NET_ADDR *server_remote )
{
	DEBUG_MSG_SERVER_NOTIFICATIONS("tc_server_notifications_init() ...\n");

	if ( init ){
		fprintf(stderr,"tc_server_notifications_init() : MODULE ALREADY INITIALIZED\n");
		return ERR_S_ALREADY_INIT;
	}

	assert( server_remote );
	assert( server_remote->name_ip );
	assert( server_remote->port );

	//Create socket for local discovery
	if ( sock_open( &notific_local_sock, LOCAL ) < 0){
		fprintf(stderr,"tc_server_notifications_init() : ERROR CREATING LOCAL SERVER SOCKET\n");
		return ERR_SOCK_CREATE;
	}

	//Set and bind to local host address
	NET_ADDR host = {SERVER_NOTIFICATIONS_LOCAL_FILE,0};
	if ( sock_bind( &notific_local_sock, &host ) ){
		fprintf(stderr,"tc_server_notifications_init() : ERROR BINDING SOCKET TO HOST ADDRESS\n");
		sock_close(&notific_local_sock);
		return ERR_SOCK_BIND_HOST;
	}

	//Create remote server socket
	if ( sock_open( &notific_remote_sock, REMOTE_UDP_GROUP ) < 0){
		fprintf(stderr,"tc_server_notifications_init() : ERROR CREATING SERVER SOCKET\n");
		return ERR_SOCK_CREATE;
	}

	//Set and bind to remote host address
	strcpy(host.name_ip,server_remote->name_ip);
	host.port = NOTIFICATIONS_GROUP_PORT;

	if ( sock_bind( &notific_remote_sock, &host ) ){
		fprintf(stderr,"tc_server_notifications_init() : ERROR BINDING SOCKET TO LOCAL ADDRESS\n");
		sock_close(&notific_local_sock);
		sock_close( &notific_remote_sock );
		return ERR_SOCK_BIND_HOST;
	}

	//Set remote group address
	NET_ADDR peer;
	strcpy(peer.name_ip, NOTIFICATIONS_GROUP_IP);
	peer.port = NOTIFICATIONS_GROUP_PORT;	
	
	//Join discovery group
	if ( sock_connect_group_tx( &notific_remote_sock, &peer ) ){
		fprintf(stderr,"tc_server_notifications_init() : ERROR REGISTERING TO DISCOVERY GROUP\n");
		sock_close(&notific_local_sock);
		sock_close( &notific_remote_sock );
		return ERR_SOCK_BIND_PEER;
	}

	init = 1;
	
	DEBUG_MSG_SERVER_NOTIFICATIONS("tc_server_notifications_init() Returning 0\n");

	return ERR_OK;
}

int tc_server_notifications_close( void )
{
	DEBUG_MSG_SERVER_NOTIFICATIONS("tc_server_notifications_close() ...\n");

	if ( !init ){
		fprintf(stderr,"tc_server_notifications_close() : MODULE ISNT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	if ( sock_close ( &notific_local_sock ) ){
		fprintf(stderr,"tc_server_notifications_close() : ERROR CLOSING LOCAL NOTIFICATIONS SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	if ( sock_close( &notific_remote_sock ) ){
		fprintf(stderr,"tc_server_notifications_close() : ERROR CLOSING REMOTE NOTIFICATIONS SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	init = 0;

	DEBUG_MSG_SERVER_NOTIFICATIONS("tc_server_notifications_close() Returning 0\n");

	return ERR_OK;
}

int tc_server_notifications_send_node_event( unsigned char event, NODE_ENTRY *node )
{
	DEBUG_MSG_SERVER_NOTIFICATIONS("tc_server_notifications_send_node_event() ...\n");

	NET_MSG notification_msg;

	assert( node );

	if ( !init ){
		fprintf(stderr,"tc_server_notifications_send_node_event() : MODULE ISNT RUNNING\n");
		return ERR_S_NOT_INIT;
	}

	notification_msg.type = DIS_MSG;
	notification_msg.event = event;
	notification_msg.node_ids[0] = node->node_id;

	//Send to local clients
	NET_ADDR peer = {CLIENT_NOTIFICATIONS_LOCAL_FILE,0};
	tc_network_send_msg( &notific_local_sock, &notification_msg, &peer );

	//Send to remote clients
	if ( tc_network_send_msg( &notific_remote_sock, &notification_msg, NULL ) ){
		fprintf(stderr,"tc_server_notifications_send_node_event() : ERROR SENDING NOTIFICATION MESSAGE\n");
		return ERR_DATA_SEND;
	}

	DEBUG_MSG_SERVER_NOTIFICATIONS("tc_server_notifications_send_node_event() Sent event %c on node id %u\n",event,node->node_id);

	return ERR_OK;
}
