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

/**	@file TC_Data_Types.c
*	@brief Source code of the functions for the data types module
*
*	This file contains the implementation of the functions for the data types module.
*	Contains a function to analyse the message types and operation and print them in a human reading way
*
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#include <stdio.h>
#include <stdlib.h>

#include "TC_Data_Types.h"
#include "TC_Error_Types.h"

int tc_msg_type_print ( MSG_TYPE type_code )
{
	switch ( type_code ){

		case REQ_MSG :
			printf(" REQ_MSG : Request type message\n");
			break;

		case ANS_MSG :
			printf(" ANS_MSG : Answer type message\n");
			break;

		case DIS_MSG :
			printf(" DIS_MSG : Discovery type message\n");
			break;

		case EVE_MSG :
			printf(" EVE_MSG : Event type message\n");
			break;	

		default :
			printf(" MESSAGE TYPE CODE NOT RECOGNIZED (%d)\n",type_code);
			return -1;
	}

	return ERR_OK;
}

int tc_op_type_print ( OP_TYPE op_code )
{
	switch ( op_code ){

		case REG_NODE :
			printf("REG_NODE\n");
			break;

		case UNREG_NODE :
			printf("UNREG_NODE\n");
			break;

		case HEART_SIG :
			printf("HEART_SIG\n");
			break;

		case REG_TOPIC :
			printf("REG_TOPIC\n");
			break;

		case DEL_TOPIC :
			printf("DEL_TOPIC\n");
			break;

		case GET_TOPIC_PROP :
			printf("GET_TOPIC_PROP\n");
			break;

		case SET_TOPIC_PROP :
			printf("SET_TOPIC_PROP\n");
			break;

		case REG_PROD :
			printf("REG_PROD\n");
			break;

		case UNREG_PROD :
			printf("UNREG_PROD\n");
			break;

		case REG_CONS :
			printf("REG_CONS\n");
			break;

		case UNREG_CONS :
			printf("UNREG_CONS\n");
			break;

		case BIND_TX :
			printf("BIND_TX\n");
			break;

		case UNBIND_TX :
			printf("UNBIND_TX\n");
			break;

		case BIND_RX :
			printf("BIND_RX\n");
			break;

		case UNBIND_RX :
			printf("UNBIND_RX\n");
			break;

		case TC_RESERV :
			printf("TC_RESERV\n");
			break;

		case TC_FREE :
			printf("TC_FREE\n");
			break;

		case TC_MODIFY :
			printf("TC_MODIFY\n");
			break;

		case REQ_ACCEPTED :
			printf("REQ_ACCEPTED\n");
			break;

		case REQ_REFUSED :
			printf("REQ_REFUSED\n");
			break;

		default :
			printf(" OPERATION TYPE CODE NOT RECOGNIZED (%d)\n",op_code);
			return -1;
	}

	return ERR_OK;
}
