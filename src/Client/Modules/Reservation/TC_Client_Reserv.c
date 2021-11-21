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

/**	@file TC_Client_Reserv.c
*	@brief Function prototypes for the client reservation module
*
*	This file contains the implementation of the functions for the client reservation module. 
*	This module is used by the management module to configure network reservations using the linux traffic control mechanism.
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

#include "TC_Client_Reserv.h"
#include "TC_Error_Types.h"
#include "Sockets.h"
#include "TC_Data_Types.h"
#include "TC_Utils.h"
#include "TC_Config.h"

/** 	@def DEBUG_MSG_CLIENT_RESERV
*	@brief If "ENABLE_DEBUG_CLIENT_RESERV" is defined debug messages related to this module are printed
*/
#if ENABLE_DEBUG_CLIENT_RESERV
#define DEBUG_MSG_CLIENT_RESERV(...) printf(__VA_ARGS__)
#else
#define DEBUG_MSG_CLIENT_RESERV(...)
#endif

static char init = 0;

static char nic_ifface[20];
static NET_ADDR server;

static unsigned int reserv_node_id = 0;

static int reserv_startup( void );
static int reserv_closeup( void );

static int tc_client_reserv_qdisc( TC_CONFIG *request );
static int tc_client_reserv_class( TC_CONFIG *request );
static int tc_client_reserv_filter( TC_CONFIG *request );

int tc_client_reserv_init( char *ifface, unsigned int node_id, NET_ADDR *server_addr )
{
	DEBUG_MSG_CLIENT_RESERV("tc_client_reserv_init() ...\n");

	if ( init ){
		fprintf(stderr,"tc_client_reserv_init() : MODULE ALREADY INITIALIZED\n");
		return ERR_C_ALREADY_INIT;
	}

	assert( ifface );
	assert( node_id );
	assert( server_addr );

	//Store NIC interface and server address	
	strcpy(nic_ifface, ifface);
	strcpy(server.name_ip,server_addr->name_ip);
	server.port = server_addr->port;

	//Init reservations
	if ( reserv_startup() ){
		fprintf(stderr,"tc_client_reserv_init() : ERROR INITIALIZING TC\n");
		return ERR_TC_INIT;
	}

	reserv_node_id = node_id;
	init = 1;
	
	DEBUG_MSG_CLIENT_RESERV("tc_client_reserv_init() Reservation module initialized\n");

	return ERR_OK;
}

int tc_client_reserv_close( void )
{
	DEBUG_MSG_CLIENT_RESERV("tc_client_reserv_close() ...\n");

	if ( !init ){
		fprintf(stderr,"tc_client_reserv_close() : MODULE ISNT RUNNING\n");
		return ERR_C_NOT_INIT;
	}

	//Free all reservations
	if ( reserv_closeup() ){
		fprintf(stderr,"tc_client_reserv_close() : ERROR FREEIN ALL RESERVATIONS\n");
		return ERR_RESERV_DEL;
	}

	init = 0;
	reserv_node_id = 0;

	DEBUG_MSG_CLIENT_RESERV("tc_client_reserv_close() Reservation module closed\n");

	return ERR_OK;
}

int tc_client_reserv_add( unsigned int topic_id, NET_ADDR *topic_addr, unsigned int req_load )
{
	DEBUG_MSG_CLIENT_RESERV("tc_client_reserv_add() Topic ID %u ...\n",topic_id);

	TC_CONFIG tc_reserv;

	if ( !init ){
		fprintf(stderr,"tc_client_reserv_add() : MODULE ISNT RUNNING\n");
		return ERR_C_NOT_INIT;
	} 
	
	assert( topic_id );
	assert( topic_addr );
	assert( req_load );

	//Set TC parameters for class
	memset(&tc_reserv,0,sizeof(tc_reserv));

	tc_reserv.operation = 'A';
	strcpy(tc_reserv.parent_handle,"1:997");
	sprintf(tc_reserv.class_id,"1:%d",topic_id); 
	tc_reserv.ceil = tc_reserv.rate = req_load;//Ceil = rate -> This disables borrowing bandwidth from other classes 
	tc_reserv.prio = 2;

	if ( tc_client_reserv_class( &tc_reserv ) ){
		fprintf(stderr,"tc_client_reserv_add() : ERROR CREATING NEW TC CLASS FOR TOPIC ID %u\n",topic_id);
		return ERR_RESERV_ADD;
	}

	//Set TC parameters for class qdisc ( we will use pfifo )
	memset(&tc_reserv,0,sizeof(tc_reserv));

	tc_reserv.operation = 'A';
	strcpy(tc_reserv.qdisc," pfifo");
	tc_reserv.qdisc_limit = PFIFO_SIZE;
	sprintf(tc_reserv.parent_handle,"1:%d",topic_id);
	sprintf(tc_reserv.handle,"1%d:",topic_id); 

	if ( tc_client_reserv_qdisc( &tc_reserv ) ){
		fprintf(stderr,"tc_client_reserv_add() : ERROR CREATING NEW QDISC FOR TC CLASS FOR TOPIC ID %u\n",topic_id);
		return ERR_RESERV_ADD;
	}

	//Set TC parameters for filter
	memset(&tc_reserv,0,sizeof(tc_reserv));

	tc_reserv.operation = 'A';
	strcpy(tc_reserv.parent_handle,"1:0");
	sprintf(tc_reserv.handle,"::%d",topic_id); 
	sprintf(tc_reserv.flow_id,"1:%d",topic_id); 
	strcpy(tc_reserv.protocol,"ip");
	strcpy(tc_reserv.dst_ip,topic_addr->name_ip);
	tc_reserv.port = topic_addr->port;
	//Filter prio is always 1 so they are all created in a known root handle (800::)
	tc_reserv.prio = 1;
					
	if ( tc_client_reserv_filter( &tc_reserv ) ){
		fprintf(stderr,"tc_client_reserv_add() : ERROR CREATING NEW TC FILTER FOR TOPIC ID %u\n",topic_id);
		return ERR_RESERV_ADD;
	}
	
	DEBUG_MSG_CLIENT_RESERV("tc_client_reserv_add() New reservation for Topic ID %u created\n",topic_id);
			
	return ERR_OK;
}

int tc_client_reserv_set( unsigned int topic_id, NET_ADDR *topic_addr, unsigned int req_load )
{
	DEBUG_MSG_CLIENT_RESERV("tc_client_reserv_set() Topic ID %u...\n",topic_id);

	TC_CONFIG tc_reserv;

	if ( !init ){
		fprintf(stderr,"tc_client_reserv_set() : MODULE ISNT RUNNING\n");
		return ERR_C_NOT_INIT;
	} 
	
	assert( topic_id );
	assert( topic_addr );
	assert( req_load );

	//Set TC parameters for class
	memset(&tc_reserv,0,sizeof(tc_reserv));

	tc_reserv.operation = 'C';
	strcpy(tc_reserv.parent_handle,"1:997");
	sprintf(tc_reserv.class_id,"1:%d",topic_id);  
	tc_reserv.ceil = tc_reserv.rate = req_load; 
	tc_reserv.prio = 2;

	if ( tc_client_reserv_class( &tc_reserv ) ){
		fprintf(stderr,"tc_client_reserv_set() : ERROR UPDATING TC CLASS OF TOPIC ID %u\n",topic_id);
		return ERR_RESERV_SET;
	}
	
	DEBUG_MSG_CLIENT_RESERV("tc_client_reserv_set() Reservation of Topic ID %u modified\n",topic_id);
			
	return ERR_OK;
}

int tc_client_reserv_del( unsigned int topic_id, NET_ADDR *topic_addr, unsigned int req_load )
{
	DEBUG_MSG_CLIENT_RESERV("tc_client_reserv_del() ...\n");

	TC_CONFIG tc_reserv;

	if ( !init ){
		fprintf(stderr,"tc_client_reserv_del() : MODULE ISNT RUNNING\n");
		return ERR_C_NOT_INIT;
	} 
	
	assert( topic_id );
	assert( topic_addr );
	assert( req_load );

	//Set TC parameters for filter
	memset(&tc_reserv,0,sizeof(tc_reserv));

	tc_reserv.operation = 'D';
	strcpy(tc_reserv.parent_handle,"1:0");
	//All filters are created under root handle 800::
	sprintf(tc_reserv.handle,"800::%d",topic_id); 
	sprintf(tc_reserv.flow_id,"1:%d",topic_id); 
	strcpy(tc_reserv.protocol,"ip");
	strcpy(tc_reserv.dst_ip,topic_addr->name_ip);
	tc_reserv.port = topic_addr->port;
	tc_reserv.prio = 1;

	if ( tc_client_reserv_filter( &tc_reserv ) ){
		fprintf(stderr,"tc_client_reserv_del() : ERROR REMOVING TC FILTER OF TOPIC ID %u\n",topic_id);
		return ERR_RESERV_DEL;
	}

	//Set TC parameters for class
	memset(&tc_reserv,0,sizeof(tc_reserv));

	tc_reserv.operation = 'D';
	strcpy(tc_reserv.parent_handle,"1:997");
	sprintf(tc_reserv.class_id,"1:%d",topic_id); 
	tc_reserv.ceil = tc_reserv.rate = req_load;
	tc_reserv.prio = 2;

	if ( tc_client_reserv_class( &tc_reserv ) ){
		fprintf(stderr,"tc_client_reserv_del() : ERROR REMOVING TC CLASS OF TOPIC ID %u\n",topic_id);
		return ERR_RESERV_DEL;
	}

	DEBUG_MSG_CLIENT_RESERV("tc_client_reserv_del() Reservation of Topic ID %u destroyed\n",topic_id);
			
	return ERR_OK;
}

static int reserv_startup( void )
{
	DEBUG_MSG_CLIENT_RESERV("reserv_startup() ... \n");

	char aux[200];

	//Initialize local TC tree
	//Create root qdisc attached to NIC -> redirects non-classified traffic to class 1:999
	sprintf(aux, "tc qdisc add dev %s root handle 1: htb default 999", nic_ifface);

	DEBUG_MSG_CLIENT_RESERV("reserv_startup() : Going to execute %s\n",aux);

	if( system(aux) < 0 ){
		fprintf(stderr,"reserv_startup() : ERROR CREATING ROOT QDISC !\n");
		return -1;
	}

	//Add root class -> required so that each children class can borrow bandwith (if desired)
	sprintf(aux, "tc class add dev %s parent 1: classid 1:997 htb rate %dMbit ceil %dMbit prio 0", nic_ifface,ROOT_BW,ROOT_BW);

	DEBUG_MSG_CLIENT_RESERV("reserv_startup() : Going to execute %s\n",aux);

	if( system(aux) < 0 ){
		fprintf(stderr,"reserv_startup() : ERROR ADDING ROOT CLASS !\n");
		return -2;
	}

	//Create class for background traffic
	sprintf(aux, "tc class add dev %s parent 1:997 classid 1:999 htb rate %dMbit ceil %dMbit prio 7", nic_ifface,BACKGROUND_BW,BACKGROUND_BW);

	DEBUG_MSG_CLIENT_RESERV("reserv_startup() : Going to execute %s\n",aux);

	if( system(aux) < 0 ){
		fprintf(stderr,"reserv_startup() : ERROR CREATING BACKGROUND TC CLASS !\n");
		return -3;
	}

	//Create class and filter for server control traffic only if server is remote (port != 0)
	if ( server.port ){
		//Create class for control traffic
		sprintf(aux, "tc class add dev %s parent 1:997 classid 1:998 htb rate %dMbit ceil %dMbit prio 1", nic_ifface,CONTROL_BW,CONTROL_BW);

		DEBUG_MSG_CLIENT_RESERV("reserv_startup() : Going to execute %s\n",aux);

		if( system(aux) < 0 ){
			fprintf(stderr,"reserv_startup() : ERROR CREATING CONTROL CLASS !\n");
			return -4;
		}
	
		//Create filter for control traffic
		sprintf(aux, "tc filter add dev %s protocol ip parent 1:0 prio 1 u32 match ip dst %s flowid 1:998", nic_ifface, server.name_ip);

		DEBUG_MSG_CLIENT_RESERV("reserv_startup() : Going to execute %s\n",aux);
	}

	if( system(aux) < 0 ){
		fprintf(stderr,"reserv_startup() : ERROR CREATING CONTROL FILTER !\n");
		return -5;
	}

	DEBUG_MSG_CLIENT_RESERV("reserv_startup() Traffic Control tree configured\n");

	return ERR_OK;
}

static int reserv_closeup( void )
{
	DEBUG_MSG_CLIENT_RESERV("reserv_closeup() ...\n");

	char aux[200];

	//Delete root qdisc (should delete all the leafs and branches too)
	sprintf(aux, "tc qdisc del dev %s root handle 1: htb", nic_ifface);

	DEBUG_MSG_CLIENT_RESERV("reserv_closeup() : Going to execute %s\n",aux);

	if( system(aux) < 0 ){
		fprintf(stderr,"reserv_closeup() : ERROR DELETING ROOT QDISC !\n");
		return -1;
	}

	DEBUG_MSG_CLIENT_RESERV("reserv_closeup() : Traffic Control tree deleted\n");

	return ERR_OK;
}

static int tc_client_reserv_qdisc( TC_CONFIG *request )
{	
	DEBUG_MSG_CLIENT_RESERV("tc_client_reserv_qdisc() ...\n");

	char command[300] = "tc qdisc";
	char aux[100];

	//Validate operation type
	switch( request->operation ){
		//Add operation		
		case 'A':
			strcat(command, " add dev ");
			break;
		//Change operation	
		case 'C':
			strcat(command, " change dev ");
			break;
		//Replace operation
		case 'R':
			strcat(command, " replace dev ");
			break;
		//Delete operation
		case 'D':
			strcat(command, " del dev ");
			break;
		default :
			fprintf(stderr,"tc_client_reserv_qdisc() : INVALID OPERATION\n");
			return -1;
	}

	//Add the device name parameter
	if( !strcmp( nic_ifface, "\0") ){
		fprintf(stderr,"tc_client_reserv_qdisc() : INVALID DEVICE NAME\n");
		return -2;
	}
	strcat(command, nic_ifface);

	//Add the parent handle parameter -> optional
	if( strcmp(request->parent_handle, "\0") ){
		if ( !strcmp(request->parent_handle, "root") )
			strcat(command, " root");
		else {
			strcat(command, " parent ");
			strcat(command, request->parent_handle);
		}
	}

	//Add the handle parameter -> optional
	if( strcmp(request->handle, "\0") ){
		strcat(command, " handle ");
		strcat(command, request->handle);	
	}

	//Setting qdisc type
	if( !strcmp( request->qdisc, "\0" ) ){
		fprintf(stderr,"tc_client_reserv_qdisc() : INVALID QDISC TYPE\n");
		return -2;
	}
	strcat(command, request->qdisc);
	
	//Set queue size -> optional
	if( request->qdisc_limit ){
		sprintf(aux, " limit %d", request->qdisc_limit);
		strcat(command, aux);
	}
	
	DEBUG_MSG_CLIENT_RESERV("tc_client_reserv_qdisc(): Going to execute \"%s\" \n",command); 

	//Execute command 
	if( system(command) < 0 ){
		fprintf(stderr,"tc_client_reserv_qdisc(): ERROR CONFIGURING TC\n");
		return -3;
	}

	DEBUG_MSG_CLIENT_RESERV("tc_client_qdisc(): TC qdisc operation successfull\n");

	return ERR_OK;
}

static int tc_client_reserv_class( TC_CONFIG *request )
{
	DEBUG_MSG_CLIENT_RESERV("tc_client_reserv_class() ...\n");

	char command[300] = "tc class";
	char aux[100];

	//Validate operation type
	switch( request->operation ){
		//Add operation		
		case 'A':
			strcat(command, " add dev ");
			break;
		//Change operation	
		case 'C':
			strcat(command, " change dev ");
			break;
		//Replace operation
		case 'R':
			strcat(command, " replace dev ");
			break;
		//Delete operation
		case 'D':
			strcat(command, " del dev ");
			break;
		default :
			fprintf(stderr,"tc_client_reserv_class(): INVALID OPERATION\n");
			return -1;
	}

	//Add the device name parameter
	if( !strcmp( nic_ifface, "\0") ){
		fprintf(stderr,"tc_client_reserv_class(): INVALID DEVICE NAME\n");
		return -1;
	}
	strcat(command, nic_ifface);

	//Add parent handle parameter
	if( !strcmp(request->parent_handle, "\0") ){
		fprintf(stderr,"tc_client_reserv_class(): INVALID PARENT HANDLE\n");
		return -2;
	}
	strcat(command, " parent ");
	strcat(command, request->parent_handle);

	//Add class id parameter -> optional
	if( strcmp(request->class_id, "\0") ){
		strcat(command, " classid ");
		strcat(command, request->class_id);
	}

	//Add rate parameter
	if( request->rate <= 0 ){
		fprintf(stderr,"tc_client_reserv_class(): INVALID RATE\n");
		return -3;
	}
	sprintf(aux, " htb rate %dbit", request->rate);
	strcat(command, aux);

	//Add ceil parameter -> optional
	if( request->ceil > 0){
		sprintf(aux, " ceil %dbit", request->ceil);
		strcat(command, aux);
	}

	//Add burst parameter -> optional
	if( request->burst > 0){
		sprintf(aux, " burst %d", request->burst);
		strcat(command, aux);
	}

	//Add cburst parameter -> optional
	if( request->cburst > 0){
		sprintf(aux, " cburst %d", request->cburst);
		strcat(command, aux);	
	}

	//Add prio parameter -> optional
	if( request->prio != 0){
		sprintf(aux, " prio %d", request->prio);
		strcat(command, aux);	
	}

	DEBUG_MSG_CLIENT_RESERV("tc_client_reserv_class(): Going to execute \"%s\" \n",command); 

	//Execute command 
	if( system(command) < 0 ){
		fprintf(stderr,"tc_client_reserv_class(): ERROR CONFIGURING TC\n");
		return -4;
	}
	
	DEBUG_MSG_CLIENT_RESERV("tc_client_reserv_class(): TC class operation successfull\n");

	return ERR_OK;
}

static int tc_client_reserv_filter( TC_CONFIG *request )
{
	DEBUG_MSG_CLIENT_RESERV("tc_client_reserv_filter() ...\n");

	char command[300] = "tc filter";
	char aux[100];

	//Validate operation type
	switch( request->operation ){
		//Add operation		
		case 'A':
			strcat(command, " add dev ");
			break;
		//Change operation	
		case 'C':
			strcat(command, " change dev ");
			break;
		//Replace operation
		case 'R':
			strcat(command, " replace dev ");
			break;
		//Delete operation
		case 'D':
			strcat(command, " del dev ");
			break;
		default :
			fprintf(stderr,"tc_client_reserv_filter(): INVALID OPERATION\n");
			return -1;
	}

	//Add the device name parameter
	if( !strcmp( nic_ifface, "\0") ){
		fprintf(stderr,"tc_client_reserv_filter(): INVALID DEVICE NAME\n");
		return -2;
	}
	strcat(command, nic_ifface);

	//Add the parent handle parameter
	if( strcmp(request->parent_handle, "\0") ){
		if (strcmp(request->parent_handle, "root") == 0)
			strcat(command, " root");
		else {
			strcat(command, " parent ");
			strcat(command, request->parent_handle);
		}
	}

	//Add protocol parameter
	if( !strcmp(request->protocol, "\0") ){
		fprintf(stderr,"tc_client_reserv_filter(): INVALID PROTOCOL TYPE\n");
		return -3;
	}
	strcat(command, " protocol ");
	strcat(command, request->protocol);

	//Add handle parameter
	if( strcmp(request->handle, "\0") ){
		strcat(command, " handle ");
		strcat(command, request->handle);		
	}

	//Add priority parameter
	if( request->prio <= 0){
		fprintf(stderr,"tc_client_reserv_filter(): INVALID PRIORITY\n");
		return -4;
	}
	sprintf(aux, " prio %d u32", request->prio);
	strcat(command, aux);

	//Dont apply the following parameters to del operations!!
	if( request->operation != 'D' ){

		if( !strcmp( request->dst_ip, "\0" ) ){
			fprintf(stderr,"tc_client_reserv_filter(): INVALID DESTINATION IP\n");
			return -6;
		}
		sprintf(aux, " match ip dst %s", request->dst_ip);
		strcat(command, aux);

		//Add flow id parameter
		if( !strcmp(request->flow_id, "\0") ){
			fprintf(stderr,"tc_client_reserv_filter(): INVALID FLOW ID\n");
			return -6;
		}
		strcat(command," flowid ");
		strcat(command, request->flow_id);
	}

	DEBUG_MSG_CLIENT_RESERV("tc_client_reserv_filter(): Going to execute \"%s\" \n",command); 

	//Execute command 
	if( system(command) < 0 ){
		fprintf(stderr,"tc_client_reserv_filter(): ERROR CONFIGURING TC\n");
		return -7;
	}

	DEBUG_MSG_CLIENT_RESERV("tc_client_reserv_filter(): TC filter operation successfull\n");

	return ERR_OK;
}
