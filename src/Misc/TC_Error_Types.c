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

/**	@file TC_Error_Types.c
*	@brief Source code of the functions for the error utility module
*
*	This file contains the implementation of the functions for the error utility module.
*	Contains a function to analyse an error code and print the type of the error in a human reading way
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#include <stdio.h>
#include <stdlib.h>

#include "TC_Error_Types.h"

int tc_error_print ( ERR_TYPE error_code )
{
	switch ( error_code ){

		case ERR_OK :
			printf(" ERR_OK : NO ERRORS\n");
			break;

		case ERR_C_NOT_INIT :
			printf(" ERR_C_NOT_INIT : CLIENT MODULE NOT INITIALIZED\n");
			break;

		case ERR_C_ALREADY_INIT :
			printf(" ERR_C_ALREADY_INIT : CLIENT MODULE ALREADY INITIALIZED\n");
			break;

		case ERR_S_NOT_INIT :
			printf(" ERR_S_NOT_INIT : SERVER MODULE NOT INITIALIZED\n");
			break;
		
		case ERR_S_ALREADY_INIT :
			printf(" ERR_S_ALREADY_INIT : SERVER MODULE ALREADY INITIALIZED\n");
			break;

		case ERR_INVALID_NIC :
			printf(" ERR_INVALID_NIC : INVALID NIC INTERFACE NAME\n");
			break;

		case ERR_SOCK_CREATE :
			printf(" ERR_SOCK_CREATE : ERROR CREATING SOCKET\n");
			break;

		case ERR_SOCK_TYPE :
			printf(" ERR_SOCK_TYPE : INVALID SOCKET TYPE\n");
			break;

		case ERR_SOCK_ENTITY : 
			printf(" ERR_SOCK_ENTITY : INVALID SOCKET ENTITY\n");
			break;

		case ERR_SOCK_INVALID_FD :
			printf(" ERR_SOCK_INVALID_FD : INVALID SOCKET FILE DESCRIPTOR\n");
			break;

		case ERR_SOCK_OPTION :
			printf(" ERR_SOCK_OPTION : ERROR SETTING SOCKET OPTION\n");
			break;

		case ERR_SOCK_BIND_HOST :
			printf(" ERR_SOCK_BIND_HOST : ERROR BINDING SOCKET TO HOST ADDRESS\n");
			break;

		case ERR_SOCK_BIND_PEER :
			printf(" ERR_SOCK_BIND_PEER : ERROR BINDING SOCKET TO PEER ADDRESS\n");
			break;

		case ERR_SOCK_CONNECT :
			printf(" ERR_SOCK_CONNECT : ERROR CONNECTING TO HOST\n");
			break;

		case ERR_TC_INIT :
			printf(" ERR_TC_INIT : ERROR INITIALIZING LINUX TRAFFIC CONTROL\n");
			break;

		case ERR_TC_CLOSE :
			printf(" ERR_TC_CLOSE : ERROR CLOSING LINUX TRAFFIC CONTROL\n");
			break;

		case ERR_SOCK_DISCONNECT :
			printf(" ERR_SOCK_DISCONNECT : ERROR DISCONNECTING FROM HOST\n");
			break;

		case ERR_SOCK_CLOSE :
			printf(" ERR_SOCK_CLOSE : ERROR CLOSING SOCKET\n");
			break;

		case ERR_COMM_INIT :
			printf(" ERR_COMM_INIT : ERROR INITIALIZING COMUNICATIONS MODULE\n");
			break;

		case ERR_DB_INIT :
			printf(" ERR_DB_INIT : ERROR INITIALIZING DATABASE MODULE\n");
			break;

		case ERR_MONIT_INIT : 
			printf(" ERR_MONIT_INIT : ERROR INITIALIZING MONITORING MODULE\n");
			break;

		case ERR_RESERV_INIT : 
			printf(" ERR_RESERV_INIT : ERROR INITIALIZING RESERVATION MODULE\n");
			break;

		case ERR_MANAG_INIT : 
			printf(" ERR_MANAG_INIT : ERROR INITIALIZING MANAGEMENT MODULE\n");
			break;

		case ERR_AC_INIT : 
			printf(" ERR_AC_INIT : ERROR INITIALIZING ADMISSION CONTROL MODULE\n");
			break;

		case ERR_DISCOVERY_INIT :
			printf(" ERR_DISCOVERY_INIT : ERROR INITIALIZING DISCOVERY MODULE\n");
			break;

		case ERR_NOTIFIC_INIT :
			printf(" ERR_NOTIFIC_INIT : ERROR INITIALIZING NOTIFICATIONS MODULE\n");
			break;

		case ERR_HANDLER_INIT : 
			printf(" ERR_HANDLER_INIT : ERROR INITIALIZING SERVER HANDLER THREAD\n");
			break;

		case ERR_COMM_CLOSE : 
			printf(" ERR_COMM_CLOSE : ERROR CLOSING COMUNICATIONS MODULE\n");
			break;

		case ERR_DB_CLOSE :
			printf(" ERR_DB_CLOSE : ERROR CLOSING DATABASE MODULE\n");
			break;

		case ERR_MONIT_CLOSE : 
			printf(" ERR_MONIT_CLOSE : ERROR CLOSING MONITORING MODULE\n");
			break;

		case ERR_RESERV_CLOSE : 
			printf(" ERR_RESERV_CLOSE : ERROR CLOSING RESERVATION MODULE\n");
			break;

		case ERR_MANAG_CLOSE : 
			printf(" ERR_MANAG_CLOSE : ERROR CLOSING MANAGEMENT MODULE\n");
			break;

		case ERR_AC_CLOSE : 
			printf(" ERR_AC_CLOSE : ERROR CLOSING ADMISSION CONTROL MODULE\n");
			break;

		case ERR_DISCOVERY_CLOSE:
			printf(" ERR_DISCOVERY_CLOSE : ERROR CLOSING DISCOVERY MODULE\n");
			break;

		case ERR_NOTIFIC_CLOSE :
			printf(" ERR_NOTIFIC_CLOSE : ERROR CLOSING NOTIFICATIONS MODULE\n");
			break;

		case ERR_HANDLER_CLOSE : 
			printf(" ERR_HANDLER_CLOSE : ERROR CLOSING SERVER HANDLER THREAD\n");
			break;

		case ERR_DISCOVERY_SERVER :
			printf(" ERR_DISCOVERY_SERVER : ERROR DISCOVERING SERVER\n");
			break;

		case ERR_INVALID_PARAM :
			printf(" ERR_INVALID_PARAM : INVALID PARAMETERS\n");
			break;

		case ERR_SEND_REQUEST :
			printf(" ERR_SEND_REQUEST : ERROR SENDING REQUEST\n");
			break;

		case ERR_GET_REQUEST : 
			printf(" ERR_GET_REQUEST : ERROR RECEIVING REQUEST\n");
			break;

		case ERR_SEND_ANSWER :
			printf(" ERR_SEND_ANSWER : ERROR SENDING ANSWER\n");
			break;
		
		case ERR_GET_ANSWER :
			printf(" ERR_GET_ANSWER : ERROR RECEIVING ANSWER\n");
			break;

		 case ERR_MEM_MALLOC :
			printf(" ERR_MEM_MALLOC : ERROR ALLOC'ING MEMORY\n");
			break;

		 case ERR_PTHREAD_CREATE :
			printf(" ERR_PTHREAD_CREATE : ERROR CREATING THREAD\n");
			break;

		 case ERR_PTHREAD_DESTROY :
			printf(" ERR_PTHREAD_DESTROY : ERROR DESTROYING THREAD\n");
			break;

		case ERR_DATA_INVALID :
			printf(" ERR_DATA_INVALID : INVALID DATA POINTER/SIZE\n");
			break;

		case ERR_DATA_TIMEOUT :
			printf(" ERR_DATA_TIMEOUT : TIMEOUT WHILE SENDING/RECEIVING DATA\n");
			break;

		case ERR_DATA_UNBLOCK :
			printf(" ERR_DATA_UNBLOCK : RECEIVED UNBLOCK SIGNAL\n");
			break;

		case ERR_DATA_SEND :
			printf(" ERR_DATA_SEND : ERROR SENDING DATA\n");
			break;

		case ERR_DATA_RECEIVE :
			printf(" ERR_DATA_RECEIVE : ERROR RECEIVING DATA\n");
			break;

		case ERR_DATA_SIZE :
			printf(" ERR_DATA_SIZE : INVALID DATA SIZE\n");
			break;
	
		case ERR_TOPIC_NOT_REG :
			printf(" ERR_TOPIC_NOT_REG : TOPIC DOES NOT EXIST\n");
			break;

		case ERR_TOPIC_DIFF_PROP :
			printf(" ERR_TOPIC_DIFF_PROP : TOPIC EXISTS WITH DIFFERENT PROPERTIES\n");
			break;

		case ERR_TOPIC_CREATE :
			printf(" ERR_TOPIC_CREATE : ERROR CREATING SERVER ENTRY FOR TOPIC\n");
			break;

		case ERR_TOPIC_DELETE :
			printf(" ERR_TOPIC_DELETE : ERROR DESTROYING SERVER ENTRY FOR TOPIC\n");
			break;

		case ERR_TOPIC_UPDATE :
			printf(" ERR_TOPIC_UPDATE : ERROR UPDATING SERVER ENTRY FOR TOPIC\n");
			break;

		case ERR_TOPIC_IN_UPDATE :
			printf(" ERR_TOPIC_IN_UPDATE : ERROR OCCURRED BECAUSE TOPIC IS UPDATING\n");
			break;

		case ERR_TOPIC_LOCAL_CREATE :
			printf(" ERR_TOPIC_LOCAL_CREATE : ERROR CREATING NODE ENTRY FOR TOPIC\n");
			break;

		case ERR_TOPIC_LOCAL_DELETE :
			printf(" ERR_TOPIC_LOCAL_DELETE : ERROR DESTROYING NODE ENTRY FOR TOPIC\n");
			break;

		case ERR_TOPIC_JOIN_TX :
			printf(" ERR_TOPIC_JOIN_TX : ERROR REGISTERING AS TOPIC PRODUCER\n");
			break;

		case ERR_TOPIC_JOIN_RX :
			printf(" ERR_TOPIC_JOIN_RX : ERROR REGISTERING AS TOPIC CONSUMER\n");
			break;

		case ERR_TOPIC_CLOSING :
			printf(" ERR_TOPIC_CLOSING : TOPIC IS BEING DESTROYED\n");
			break;

		case ERR_REG_NODE :
			printf(" ERR_REG_NODE : ERROR REGISTERING NODE IN THE NETWORK\n");
			break;
		
		case ERR_NODE_DELETE :
			printf(" ERR_NODE_DELETE : ERROR DELETING NODE ENTRY\n");
			break;

		case ERR_UNREG_NODE :
			printf(" ERR_UNREG_NODE : ERROR UNREGISTERING NODE FROM THE NETWORK\n");
			break;

		case ERR_NODE_BIND :
			printf(" ERR_NODE_BIND : ERROR WHILE BINDING NODE(S)\n");
			break;

		case ERR_NODE_UNBIND :
			printf(" ERR_NODE_UNBIND : ERROR WHILE UNBINDING NODE(S)\n");
			break;

		case ERR_NODE_MAX :
			printf(" ERR_NODE_MAX : MAXIMUM NUMBER OF NODES EXCEEDED\n");
			break;

		case ERR_NODE_DIFF_ADDR :
			printf(" ERR_NODE_DIFF_ADDR : NODE WITH SAME ID REGISTERED WITH DIFFERENT ADDRESS\n");
			break;

		case ERR_NODE_NOT_REG :
			printf(" ERR_NODE_NOT_REG : NODE IS NOT REGISTERED IN THE NETWORK\n");
			break;

		case ERR_NODE_NOT_REG_TX :
			printf(" ERR_NODE_NOT_REG_TX : NODE IS NOT REGISTERED AS PRODUCER OF TOPIC\n");
			break;

		case ERR_NODE_NOT_REG_RX :
			printf(" ERR_NODE_NOT_REG_RX : NODE IS NOT REGISTERED AS CONSUMER OF TOPIC\n");
			break;
		
		case ERR_NODE_PROD_BW :
			printf(" ERR_NODE_PROD_BW : PRODUCER NODES DON'T HAVE ENOUGH BANDWIDTH\n");
			break;

		case ERR_NODE_CONS_BW :
			printf(" ERR_NODE_CONS_BW : CONSUMER NODES DON'T HAVE ENOUGH BANDWIDTH\n");
			break;

		case ERR_NODE_PROD_RESERV :
			printf(" ERR_NODE_PROD_RESERV : PRODUCER NODE FAILED TO RESERV BANDWIDTH\n");
			break;

		case ERR_NODE_CONS_RESERV :
			printf(" ERR_NODE_CONS_RESERV : CONSUMER NODE FAILED TO RESERV BANDWIDTH\n");
			break;

		case ERR_NODE_PROD_F_RESERV :
			printf(" ERR_NODE_PROD_F_RESERV : PRODUCER NODE FAILED TO FREE RESERVED BANDWIDTH\n");
			break;

		case ERR_NODE_CONS_F_RESERV :
			printf(" ERR_NODE_CONS_F_RESERV : CONSUMER NODE FAILED TO FREE RESERVED BANDWIDTH\n");
			break;

		case ERR_NODE_PROD_REG :
			printf(" ERR_NODE_PROD_REG : NODE REGISTRATION AS PRODUCER OF TOPIC FAILED\n");
			break;

		case ERR_NODE_CONS_REG :
			printf(" ERR_NODE_CONS_REG : NODE REGISTRATION AS CONSUMER OF TOPIC FAILED\n");
			break;

		case ERR_NODE_PROD_UNREG :
			printf(" ERR_NODE_PROD_UNREG : NODE UNREGISTRATION AS PRODUCER OF TOPIC FAILED\n");
			break;

		case ERR_NODE_CONS_UNREG :
			printf(" ERR_NODE_CONS_REG : NODE UNREGISTRATION AS CONSUMER OF TOPIC FAILED\n");
			break;

		case ERR_BIND_TX_TIMEDOUT :
			printf(" ERR_BIND_TX_TIMEDOUT : TIME-OUT WHILE WAITING FOR BIND AS PRODUCER OF TOPIC\n");
			break;
		
		case ERR_BIND_RX_TIMEDOUT :
			printf(" ERR_BIND_RX_TIMEDOUT : TIME-OUT WHILE WAITING FOR BIND AS CONSUMER OF TOPIC\n");
			break;

		case ERR_NODE_NOT_BOUND_TX :
			printf(" ERR_NODE_NOT_BOUND_TX : NODE IS NOT BOUND AS PRODUCER OF TOPIC\n");
			break;

		case ERR_NODE_NOT_BOUND_RX :
			printf(" ERR_NODE_NOT_BOUND_RX : NODE IS NOT BOUND AS CONSUMER OF TOPIC\n");
			break;

		case ERR_RESERV_ADD :
			printf(" ERR_RESERV_ADD : FAILED TO CREATE RESERVATION ON NODE(S)\n");
			break;

		case ERR_RESERV_DEL :
			printf(" ERR_RESERV_DEL : FAILED TO DESTROY RESERVATION ON NODE(S)\n");
			break;

		case ERR_RESERV_SET :
			printf(" ERR_RESERV_SET : FAILED TO MODIFY RESERVATION ON NODE(S)\n");
			break;

		case ERR_THREAD_CREATE :
			printf(" ERR_THREAD_CREATE : ERROR CREATING THREAD\n");
			break;

		case ERR_THREAD_DESTROY :
			printf(" ERR_THREAD_DESTROY : ERROR DESTROYING THREAD\n");
			break;

		case ERR_THREAD_TIMEOUT :
			printf(" ERR_THREAD_TIMEOUT : TIMEDOUT WAINTING FOR THREAD TO LOCK FEEDBACK MUTEX\n");
			break;

		default :
			printf(" ERROR CODE NOT RECOGNIZED (%d)\n",error_code);
			return -1;
	}

	return 0;
}

