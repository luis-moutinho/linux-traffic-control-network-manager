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

/**	@file TC_Utils.c
*	@brief Source code of the utilities functions
*
*	This file contains the implementation of the utilities functions. These functions
*	aid in threads creation, NIC IP address retrieval and the transformation of data messages
*	to host-to-network or network-to-host format
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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <pthread.h>

#include "Sockets.h"
#include "TC_Data_Types.h"
#include "TC_Config.h"
#include "TC_Utils.h"
#include "TC_Error_Types.h"

#if ENABLE_MSG_TC_UTILS
#define DEBUG_MSG_TC_UTILS(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG_TC_UTILS(...)
#endif

static int net_msg_set_host_to_network(  NET_MSG *msg, NET_MSG *ret_msg );
static int net_msg_set_network_to_host( NET_MSG *msg, NET_MSG *ret_msg );

int tc_network_send_msg( SOCK_ENTITY *sock, NET_MSG *msg, NET_ADDR *peer )
{
	DEBUG_MSG_TC_UTILS("tc_network_send_msg() ...\n");

	NET_MSG network_msg;

	assert(sock);
	assert(msg);

	net_msg_set_host_to_network( msg, &network_msg );

	//Send message to peer
	if ( sock_send( sock, peer, (char *)&network_msg, sizeof(NET_MSG) ) < 0 ){
		//perror("tc_network_send_msg() : ERROR SENDING MESSAGE --");
		return ERR_DATA_SEND;
	}

	DEBUG_MSG_TC_UTILS("tc_network_send_msg() Message sent\n");

	return ERR_OK;
}

int tc_network_get_msg( SOCK_ENTITY *sock, unsigned int timeout, NET_MSG *ret_msg, NET_ADDR *ret_sender )
{
	DEBUG_MSG_TC_UTILS("tc_network_get_msg() ...\n");

	int ret;
	NET_MSG network_msg;

	assert( sock );
	assert( ret_msg );

	//Get message from sender
	NET_ADDR sender;

	if ( (ret = sock_receive( sock, NULL, timeout, (char *)&network_msg, 0, &sender )) < 0 ){
		if ( ret != ERR_DATA_TIMEOUT ) perror("tc_network_get_msg() : ERROR RECEIVING MESSAGE --\n");
		return ERR_DATA_RECEIVE;
	}

	if ( ret_sender ){
		strcpy(ret_sender->name_ip,sender.name_ip);
		ret_sender->port = sender.port;
	}

	net_msg_set_network_to_host( &network_msg, ret_msg );

	DEBUG_MSG_TC_UTILS("tc_network_get_msg() Returning with request message\n");

	return ERR_OK;
}

int tc_network_get_nic_ip( char *ifface , char *ret_ip )
{
	DEBUG_MSG_TC_UTILS("tc_network_get_nic_ip() ...\n");

	int fd;
	struct ifreq ifr;

	assert( ifface );
	assert( ret_ip );

	if ( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ){
		fprintf(stderr,"tc_network_get_nic_ip() : ERROR CREATING SOCKET\n");
		return ERR_SOCK_CREATE;
	} 

	//Want to get an IPv4 IP address
	ifr.ifr_addr.sa_family = AF_INET;

	//Want IP address attached to ifface
	strncpy(ifr.ifr_name, ifface, IFNAMSIZ-1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	close(fd);

	strcpy(ret_ip,inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

	DEBUG_MSG_TC_UTILS("tc_network_get_nic_ip() NIC %s ip address is %s\n",ifface,ret_ip);

	return ERR_OK;
}

int tc_thread_create( void *thread_call, pthread_t *ret_thread_id, char *ret_quit_flag, pthread_mutex_t *ret_thread_lock, unsigned int timeout )
{
	DEBUG_MSG_TC_UTILS("tc_thread_create() ...\n");

	int tries = timeout;

	assert( thread_call );
	assert( ret_thread_id );
	assert( ret_quit_flag );

	//Set quit flag
	*ret_quit_flag = THREAD_RUN;

	if ( ret_thread_lock ){
		//Running feedback is requested

		//Initialize thread mutex
		pthread_mutex_init( ret_thread_lock, NULL );

		//Create thread
		if ( pthread_create( ret_thread_id, NULL, thread_call, NULL ) ){
			fprintf(stderr,"tc_thread_create() : ERROR STARTING CREATING THREAD\n");
			*ret_quit_flag = THREAD_STOP;
			pthread_mutex_destroy( ret_thread_lock );
			return ERR_THREAD_CREATE;
		}

		//Wait until thread locks mutex
		while ( !pthread_mutex_trylock( ret_thread_lock) && (tries >= 0) ){
			//Mutex wasn't locked by thread -> thread not running yet
			pthread_mutex_unlock( ret_thread_lock );
			usleep(1000);
			tries--;
		}

		if ( tries < 0 ){
			fprintf(stderr,"tc_thread_create() : TIMEDOUT WAITING FOR THREAD TO LOCK MUTEX -- GOING TO DESTROY IT\n");
			*ret_quit_flag = THREAD_STOP;
			pthread_cancel( *ret_thread_id );
			pthread_mutex_destroy( ret_thread_lock );
			return ERR_THREAD_TIMEOUT;
		}
	}else{
		//Running feedback is not requested
		if ( pthread_create( ret_thread_id, NULL, thread_call, NULL ) ){
			fprintf(stderr,"tc_thread_create() : ERROR STARTING CREATING THREAD\n");
			*ret_quit_flag = THREAD_STOP;
			return ERR_THREAD_CREATE;
		}
	}

	DEBUG_MSG_TC_UTILS("tc_thread_create() Thread created -- Assigned id %d\n",*thread_id);

	return ERR_OK;
}
 
int tc_thread_destroy( pthread_t *thread_id, char *quit_flag, pthread_mutex_t *thread_lock, unsigned int timeout )
{
	DEBUG_MSG_TC_UTILS("tc_thread_destroy() ...\n");

	struct timespec wait;

	assert( thread_id );
	assert( quit_flag );

	//Stop monitoring thread
	*quit_flag = THREAD_STOP;

	if ( thread_lock ){
		//Thread has feedback mutex

		//Wait for thread to end -> if time expires kill it	
		memset(&wait,0, sizeof(struct timespec));
		wait.tv_nsec = timeout*1000000;

		if ( pthread_mutex_timedlock( thread_lock, &wait ) ){
			fprintf(stderr,"tc_thread_destroy() : TIME-OUT WAITING FOR THREAD TO END\n");
			pthread_cancel( *thread_id );	
		}

		//Destroys mutex
		pthread_mutex_destroy( thread_lock );
	}

	//Wait for thread to end
	pthread_join( *thread_id, NULL );

	DEBUG_MSG_TC_UTILS("tc_thread_destroy() Thread with ID %d destroyed\n",*thread_id);

	return ERR_OK;
}


	unsigned char type;			/**< The type of message (REQUEST/ANSWER) */
	unsigned char op_type;			/**< The type of operation (REG_NODE/UNREG_NODE/REG_TOPIC/...) */
	unsigned char event;			/**< The event type (NODE_PLUG,NODE_UNPLUG) */
	int error;				/**< The error code occured while handling a request */
/*@}*/


/*@}*//**
* @name Node Information
*//*@{*/
	unsigned int node_ids[MAX_MULTI_NODES];	/**< The involved nodes ID (I.E for a bind request that requests several nodes to bind at the same time)*/
	unsigned int n_nodes;			/**< The number of involved nodes */

/*@}*//**
* @name Topic Properties Information
*//*@{*/

	NET_ADDR topic_addr;			/**< The topic network address */			
	unsigned int topic_id;			/**< The ID of the topic */
	unsigned int topic_load;		/**< The topics load/requesting load */
	unsigned int channel_size;		/**< The maximum size of the messages sent through the topic*/
	unsigned int channel_period;


static int net_msg_set_host_to_network( NET_MSG *msg, NET_MSG *ret_msg )
{
	DEBUG_MSG_TC_UTILS("client_ac_req_set_host_to_network() ...\n");

	int i;

	assert( msg );
	assert( ret_msg );

	ret_msg->type		= msg->type;
	ret_msg->op		= msg->op;
	ret_msg->event		= msg->event;
	ret_msg->error		= htonl(msg->error);
	
	for( i = 0; i<MAX_MULTI_NODES; i++ ){
		ret_msg->node_ids[i] = (unsigned int) htonl(msg->node_ids[i]);
	}
	ret_msg->n_nodes	= (unsigned int) htonl(msg->n_nodes);

	ret_msg->topic_id 	= (unsigned int) htonl(msg->topic_id);
	memcpy(ret_msg->topic_addr.name_ip,msg->topic_addr.name_ip,sizeof(msg->topic_addr.name_ip));
	ret_msg->topic_addr.port= (unsigned int) htonl(msg->topic_addr.port);
	ret_msg->topic_load 	= (unsigned int) htonl(msg->topic_load);
	ret_msg->channel_size 	= (unsigned int) htonl(msg->channel_size);
	ret_msg->channel_period = (unsigned int) htonl(msg->channel_period);

	DEBUG_MSG_TC_UTILS("client_ac_req_set_host_to_network() Returning 0\n");

	return ERR_OK;
}

static int net_msg_set_network_to_host( NET_MSG *msg, NET_MSG *ret_msg )
{
	DEBUG_MSG_TC_UTILS("client_ac_req_set_network_to_host() ...\n");

	int i;

	assert( msg );
	assert( ret_msg );

	ret_msg->type		= msg->type;
	ret_msg->op		= msg->op;
	ret_msg->event		= msg->event;
	ret_msg->error		= htonl(msg->error);
	
	for( i = 0; i<MAX_MULTI_NODES; i++ ){
		ret_msg->node_ids[i] = (unsigned int) ntohl(msg->node_ids[i]);
	}
	ret_msg->n_nodes	= (unsigned int) ntohl(msg->n_nodes);

	ret_msg->topic_id 	= (unsigned int) ntohl(msg->topic_id);
	memcpy(ret_msg->topic_addr.name_ip,msg->topic_addr.name_ip,sizeof(msg->topic_addr.name_ip));
	ret_msg->topic_addr.port= (unsigned int) ntohl(msg->topic_addr.port);
	ret_msg->topic_load 	= (unsigned int) ntohl(msg->topic_load);
	ret_msg->channel_size 	= (unsigned int) ntohl(msg->channel_size);
	ret_msg->channel_period = (unsigned int) ntohl(msg->channel_period);


	DEBUG_MSG_TC_UTILS("client_ac_req_set_network_to_host() Returning 0\n");

	return ERR_OK;
}
