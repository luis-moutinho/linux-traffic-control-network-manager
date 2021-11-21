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

/**	@file TC_API.c
*	@brief Source code for a simple application example
*
*	This file contains a simple example to show how to use both the server and client APIs in an application.
*	In this simple example producer nodes will send their node ID and print 'S node_id' while consumer nodes will
*	receive the messages with the producer node ID and print 'R producer_node_id'.
*	The usage is simple and theres an example which can be seen in the help menu (run './TC_API.test -h' in a terminal).
*	Supports multiple producer and consumer nodes as well as producer-consumer nodes.
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>             /* getopt_long() */
#include <signal.h>
#include <pthread.h>

//For get_nic
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "TC_Client.h"
#include "TC_Server.h"
#include "TC_Error_Types.h"

//#define CHANGES //To enable/disable some dynamic changes triggered by nodes 

#define PDEBUG(FORMAT,...) 	printf("%s:%s(%d) " FORMAT "\n",__FILE__, __func__, __LINE__, __VA_ARGS__);

//Just to ease code
static char node_type = 's';
static int quit = 0;
static int rate = 1000,size = 6000,period = 1000;
static char ifface[10] = "eth0";
static int server_port = 5000;
static unsigned int topic_id = 1;
static unsigned int node_id = 0;
static unsigned int topic_size, topic_period;
static pthread_t cons_thread_id,prod_thread_id, notice_thread_id;

static void sigfun(int sig);
static signed char get_nic_ip( char *ifface , char *ret_ip );
static void consumer( void );
static void producer( void );
static void notifications( void );

static const char short_options[] = "i:t:n:r:c:h:";
static const struct option long_options[] = {
	{ "interface", 				required_argument, 	NULL, 	'i' },
	{ "type", 				required_argument, 	NULL, 	't' },
	{ "node_id", 				required_argument, 	NULL, 	'n' },
	{ "rate", 				required_argument, 	NULL, 	'r' },
	{ "changes", 				required_argument, 	NULL, 	'c' },
	{ "help", 				no_argument, 		NULL, 	'h' },
	{ 0, 0, 0, 0 }
};

static void usage(FILE * fp, char ** argv) {
	fprintf(fp,
			"\nUsage: %s [options]\n\n"
			"Options:\n"
			"-i | --interface	NIC interface to be used 			(default eth0)\n"
			"-t | --type		Producer[p]/Consumer[c]/Cons-Prod[m]/Server[s]	(default s)\n"
			"-n | --node_id		Node Id						(default 0 -> random)\n"
			"-r | --rate		Producer rate in ms				(default 1000)\n"
			"-c | --changes		Enable (1) some dynamic changes by the nodes 	(default 0)\n"
			"-h | --help		Print this message\n"
			"\nNOTE : Topic is created with period = 1000 ms. You can force the producers to produce at i.e. 500ms with the option -r to check the traffic enforcement\n"
			"\nExample with 4 nodes ( 1 Server node, 1 producer node, 1 consumer node, 1 producer-consumer node)\n"
			"Start server node	- './TC_API.test -i eth1'\n"
			"Start producer node 	- './TC_API.test -i eth1 -t p'\n"
			"Start consumer node	- './TC_API.test -i eth1 -t c'\n"
			"Start prod-cons node	- './TC_API.test -i eth1 -t m'\n"
			"\nProducer nodes will send their node ID and print 'S node_id'\n"
			"Consumer nodes will receive producer node ID and print 'R producer_node_id'\n\n"
			"", argv[0]);
	return;
}

int main (int argc, char* argv[])
{
	//PDEBUG("%d teste",12);
	int c,index;
	int count = 0;
	int ret = 0;
	int changes = 0;

	(void) signal(SIGINT, sigfun);

	while ( (c = getopt_long(argc, argv, short_options, long_options, &index)) != -1 ){
		
		switch (c) {
			case 0: /* getopt_long() flag */
				break;

			case 'i':
				strcpy(ifface,optarg);
				break;

			case 't':
				//Check if param is valid for option
				if( strcmp(optarg,"p") && strcmp(optarg,"c") && strcmp(optarg,"m") && strcmp(optarg,"s") ){
					fprintf(stderr,"Invalid value for option --type -> Node type : Producer[p]/Consumer[c]/Cons-Prod[m]/Server[s]\n");
					return -1;
				}
				if( !strcmp(optarg,"c") ) node_type = 'c';
				if( !strcmp(optarg,"p") ) node_type = 'p';
				if( !strcmp(optarg,"m") ) node_type = 'm';
				break;
			case 'n':
				node_id = atoi(optarg);
				break;

			case 'r':
				rate = atoi(optarg);
				break;

			case 'c':
				changes = atoi(optarg);
				break;
		
			case 'h':
				usage(stdout, argv);
				return 0;
				
			default:
				usage(stderr, argv);
				return -1;
		}
	}
	
	printf("\n Configuration \n");
	printf("NIC interface %s\n",ifface);
	printf("Node type %c\n",node_type);
	printf("Node Id %d\n",node_id);

	printf("Topic Id %d Size %d Period %d\n",topic_id,size,period);
	printf("Producer rate %d\n",rate);


	//CONSUMER NODE
	if ( node_type == 'c' ){

		if( (ret = tc_client_init( ifface, node_id )) > 0 ){
			node_id = (unsigned int) ret;
			printf("Server assigned to this node the ID %u\n",node_id);
		}else{
			fprintf(stderr,"Error initializing node\n");
			tc_error_print( ret );
			return -2;
		}

		if ( !tc_client_topic_get_prop( topic_id , &topic_size, &topic_period ) )
			printf("Topic Id %d exists with size %d period %d\n",topic_id,topic_size,topic_period);

		tc_client_topic_create( topic_id, size, period );
		tc_client_register_rx( topic_id );
		
		if ( pthread_create( &cons_thread_id, NULL, (void *)consumer, NULL ) != 0){
			fprintf(stderr,"main() : ERROR STARTING CONSUMER THREAD\n");
			tc_client_close();
			return -2;
		}

	}


	//PRODUCER NODE
	if ( node_type == 'p' ){
		
		if( (ret = tc_client_init( ifface, node_id )) > 0 ){
			node_id = (unsigned int) ret;
			printf("Server assigned to this node the ID %u\n",node_id);
		}else{
			fprintf(stderr,"Error initializing node\n");
			tc_error_print( ret );	
			return -2;
		}

		if ( ! tc_client_topic_get_prop( topic_id , &topic_size, &topic_period ) )
			printf("Topic Id %d exists with size %d period %d\n",topic_id,topic_size,topic_period);

		tc_client_topic_create( topic_id, size, period );
		tc_client_register_tx ( topic_id );
		
		if ( pthread_create( &prod_thread_id, NULL, (void *)producer, NULL ) != 0){
			fprintf(stderr,"main() : ERROR STARTING PRODUCER THREAD\n");
			tc_client_close();
			return -2;
		}

	}


	//CONSUMER-PRODUCER NODE
	if ( node_type == 'm' ){

		if( (ret = tc_client_init( ifface, node_id )) > 0 ){
			node_id = (unsigned int) ret;
			printf("Server assigned to this node the ID %u\n",node_id);
		}else{
			fprintf(stderr,"Error initializing node\n");
			tc_error_print( ret );
			return -2;
		}

		if ( ! tc_client_topic_get_prop( topic_id , &topic_size, &topic_period ) )
			printf("Topic Id %d exists with size %d period %d\n",topic_id,topic_size,topic_period);

		tc_client_topic_create( topic_id, size, period );

		tc_client_register_tx ( topic_id );
		tc_client_register_rx( topic_id );
		
		if ( pthread_create( &cons_thread_id, NULL, (void *)consumer, NULL ) != 0){
			fprintf(stderr,"main() : ERROR STARTING CONSUMER THREAD\n");
			tc_client_close();
			return -2;
		}

		if ( pthread_create( &prod_thread_id, NULL, (void *)producer, NULL ) != 0){
			fprintf(stderr,"main() : ERROR STARTING PRODUCER THREAD\n");
			tc_client_close();
			return -2;
		}

	}


	//SERVER NODE
	if ( node_type == 's' ){

		if ( (ret = tc_server_init( ifface, server_port )) ){
			fprintf(stderr,"Error initializing server\n");
			tc_error_print( ret );
			return -2;
		}
			
	}else{

		//Create notifications thread
		if ( pthread_create( &notice_thread_id, NULL, (void *)notifications, NULL ) != 0){
			fprintf(stderr,"main() : ERROR STARTING NOTIFICATIONS THREAD\n");
			tc_client_close();
			return -2;
		}

	}

	while(!quit){

		usleep(1000000);
		count++;

		if ( changes ){
			//Some dynamic changes to the system
			if ( count == 10 && node_type == 'm'){
				printf("\nM - GOING TO UNREGISTER AS TX\n");
				tc_client_unregister_tx( topic_id );
			}

			if ( count == 15 && node_type == 'm'){
				printf("\nM - GOING TO REGISTER AS TX\n");
				tc_client_register_tx( topic_id );
			}

			if ( count == 20 && node_type == 'p'){
				printf("\nP - GOING TO DESTROY TOPIC \n");
				tc_client_topic_destroy( topic_id );
			}

			if ( count == 25 && node_type == 'p'){
				printf("\nP - GOING TO CREATE TOPIC \n");
				tc_client_topic_create( topic_id, size, period );
			}

			if ( count == 45 && node_type == 'c'){
				printf("\nC - GOING TO UPDATE TOPIC \n");
				tc_client_topic_set_prop( topic_id , size, period/10 );
			}

			if ( count >= 50 )
				count = 0;
		}
	}		

	if ( node_type == 'c' || node_type == 'p' || node_type == 'm' )
		tc_client_close();
	else{
		printf("\n");
		tc_server_close();	
	}

	return 0;
}

static void sigfun(int sig) 
{
	printf("sigfun() ... CTRL+C Pressed\n");

	quit = 1;
	
	sleep(1);

	if ( node_type == 'c' || node_type == 'p' || node_type == 'm' )
		tc_client_close();
	else{
		tc_server_close();	
	}

	(void) signal(SIGINT, SIG_DFL);

	printf("sigfun() Returning\n");

	exit(sig);
}

static void consumer( void ) 
{
	char buffer[100000];
	int err;
	tc_client_bind_rx( topic_id, 0 );

	while( !quit ){

		if ( ( err = tc_client_topic_receive( topic_id, 0, buffer )) < 0 ){

			tc_error_print( err );

			memset(buffer,0,sizeof(buffer));
				
			if ( err == ERR_TOPIC_CLOSING )
				usleep(10000);

			if ( err == ERR_TOPIC_IN_UPDATE )
				usleep(10000);

			if ( err == ERR_NODE_NOT_REG_RX )
				if ( (err = tc_client_register_rx ( topic_id )) == ERR_TOPIC_NOT_REG )
					usleep(2000000);

			if ( err == ERR_NODE_NOT_BOUND_RX )
				if ( (err = tc_client_bind_rx( topic_id, 0 )) == ERR_TOPIC_NOT_REG )
					usleep(2000000);

			usleep(10000);
		}else{
			printf(" R%s",buffer);
			fflush(stdout);
		}	
	}

	return;
}

static void producer( void ) 
{
	char buffer[100000];
	char nic_ip[50];
	int err;
	get_nic_ip( ifface , nic_ip );

	sprintf( buffer, "%d", node_id );
	tc_client_bind_tx( topic_id, 0 );

	while ( !quit ){

		if ( (err = tc_client_topic_send( topic_id, buffer, size )) < 0 ){

			tc_error_print( err );

			if ( err == ERR_TOPIC_CLOSING )
				usleep(10000);

			if ( err == ERR_TOPIC_IN_UPDATE )
				usleep(10000);

			if ( err == ERR_NODE_NOT_REG_TX ){
				if ( (err = tc_client_register_tx ( topic_id )) == ERR_TOPIC_NOT_REG )
					usleep(2000000);
			}

			if ( err == ERR_NODE_NOT_BOUND_TX ){
				if ( (err = tc_client_bind_tx( topic_id, 0 )) == ERR_TOPIC_NOT_REG )
					usleep(2000000);
			}
		}else{
			printf(" S%s",buffer);
			fflush(stdout);
		}

		usleep( rate * 1000 );
	}

	return;	
}

static void notifications( void ) 
{
	unsigned char event;
	unsigned int node_id = 0;

	while ( !quit ){

		if ( !tc_client_get_node_event( 100, &event, &node_id ) ){
			printf("\nGot notification of event %d on node id %u\n",(int)event,node_id);
		}

	}

	return;
}

static signed char get_nic_ip( char *ifface , char *ret_ip )
{
	int fd;
	struct ifreq ifr;

	if ( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ){
		fprintf(stderr,"get_nic_ip() : ERROR CREATING SOCKET\n");
		return -1;
	} 

	//Want to get an IPv4 IP address
	ifr.ifr_addr.sa_family = AF_INET;

	//Want IP address attached to ifface
	strncpy(ifr.ifr_name, ifface, IFNAMSIZ-1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	close(fd);

	strcpy(ret_ip,inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

	return 0;
}
