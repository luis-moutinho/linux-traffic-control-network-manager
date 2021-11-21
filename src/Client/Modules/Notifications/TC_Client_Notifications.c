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

/**	@file TC_Client_Notifications.c
*	@brief Source code of the functions for the client notifications module
*
*	This file contains the implementation of the functions for the client notifications module. 
*	This module polls for notification messages sent by the server notifications module when an event occurs (I.E node registered in the network).
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

#include "TC_Client_Notifications.h"

#include "Sockets.h"
#include "TC_Data_Types.h"
#include "TC_Utils.h"
#include "TC_Config.h"
#include "TC_Error_Types.h"

static char init = 0;
static SOCK_ENTITY sock;

/** 	@def DEBUG_MSG_CLIENT_NOTIFICATIONS
*	@brief If "ENABLE_DEBUG_CLIENT_NOTIFICATIONS" is defined debug messages related to this module are printed
*/
#if ENABLE_DEBUG_CLIENT_NOTIFICATIONS
#define DEBUG_MSG_CLIENT_NOTIFICATIONS(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG_CLIENT_NOTIFICATIONS(...)
#endif

int tc_client_notifications_init( char *ifface, NET_ADDR *server )
{
	DEBUG_MSG_CLIENT_NOTIFICATIONS("tc_client_notifications_init() ...\n");

	char nic_ip[50];

	if ( init ){
		fprintf(stderr,"tc_client_notifications_init() : MODULE ALREADY INITIALIZED\n");
		return ERR_C_ALREADY_INIT;
	}

	assert( ifface );
	assert( server );

	//Get nic ip address
	if ( tc_network_get_nic_ip( ifface, nic_ip ) < 0){
		fprintf(stderr,"tc_client_notifications_init() : ERROR GETTING NIC IP ADDRESS\n");
		return ERR_INVALID_NIC;
	}

	if ( !server->port ){
		//Server is in the same local node

		//Create local server socket
		if ( sock_open( &sock, LOCAL ) < 0){
			fprintf(stderr,"tc_client_notifications_init() : ERROR CREATING LOCAL SERVER SOCKET\n");
			return ERR_SOCK_CREATE;
		}

		//Set and bind to host address
		NET_ADDR host = {CLIENT_NOTIFICATIONS_LOCAL_FILE,0};
		if ( sock_bind( &sock, &host ) ){
			fprintf(stderr,"tc_client_notifications_init() : ERROR BINDING SOCKET TO HOST ADDRESS\n");
			sock_close(&sock);
			return ERR_SOCK_BIND_HOST;
		}

		//Connect socket to server
		NET_ADDR peer = {SERVER_NOTIFICATIONS_LOCAL_FILE,0};
		if ( sock_connect_peer( &sock, &peer ) ){
			fprintf(stderr,"tc_client_notifications_init() : ERROR CONNECTING LOCAL SOCKET TO SERVER\n");
			sock_close ( &sock );
			return ERR_SOCK_BIND_PEER;
		}

	}else{
		//Server is in a remote node

		//Create remote server socket
		if ( sock_open( &sock, REMOTE_UDP_GROUP ) < 0){
			fprintf(stderr,"tc_client_notifications_init() : ERROR CREATING SERVER SOCKET\n");
			return ERR_SOCK_CREATE;
		}

		//Set and bind to host address
		NET_ADDR host;
		strcpy(host.name_ip,nic_ip);
		host.port = NOTIFICATIONS_GROUP_PORT;

		if ( sock_bind( &sock, &host ) ){
			fprintf(stderr,"tc_client_notifications_init() : ERROR BINDING SOCKET TO LOCAL ADDRESS\n");
			sock_close( &sock );
			return ERR_SOCK_BIND_HOST;
		}

		//Set remote address
		NET_ADDR peer;
		strcpy(peer.name_ip, NOTIFICATIONS_GROUP_IP);
		peer.port = NOTIFICATIONS_GROUP_PORT;

		if ( sock_connect_group_rx( &sock, &peer ) ){
			fprintf(stderr,"tc_client_notifications_init() : ERROR REGISTERING TO NOTIFICATIONS GROUP\n");
			sock_close( &sock );
			return ERR_SOCK_CONNECT;
		}
	}

	init = 1;

	DEBUG_MSG_CLIENT_NOTIFICATIONS("tc_client_notifications_init() Management module initialized\n");

	return ERR_OK;
}

int tc_client_notifications_close( void )
{
	DEBUG_MSG_CLIENT_NOTIFICATIONS("tc_client_notifications_close() ...\n");

	if ( !init ){
		fprintf(stderr,"tc_client_notifications_close() : MODULE ISNT RUNNING\n");
		return ERR_C_NOT_INIT;
	}
		
	if ( sock_close( &sock ) ){
		fprintf(stderr,"tc_client_notifications_close() : ERROR CLOSING REMOTE SOCKET\n");
		return ERR_SOCK_CLOSE;
	}

	init = 0;

	DEBUG_MSG_CLIENT_NOTIFICATIONS("tc_client_notifications_close() Notifications module closed\n");

	return ERR_OK;
}

int tc_client_notifications_get( unsigned int timeout, NET_MSG *ret_msg )
{
	DEBUG_MSG_CLIENT_NOTIFICATIONS("tc_client_notifications_get() ...\n");

	int ret = 0;

	if ( !init ){
		fprintf(stderr,"tc_client_notifications_get() : MODULE ISNT RUNNING\n");
		return ERR_C_NOT_INIT;
	}

	if ( !ret_msg ){
		fprintf(stderr,"tc_client_notifications_get() : INVALID RET_MSG\n");
		return ERR_INVALID_PARAM;
	}

	if ( (ret = tc_network_get_msg( &sock, timeout, ret_msg, NULL )) ){
		//fprintf(stderr,"tc_client_notifications_get() : COULDN'T GET NOTIFICATIONS MESSAGE\n");
		return ret;
	}

	DEBUG_MSG_CLIENT_NOTIFICATIONS("tc_client_notifications_get() Got event %c on node id %u\n",ret_msg->op,ret_msg->node_ids[0]);

	return ERR_OK;
}
