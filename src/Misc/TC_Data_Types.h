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

/**	@file TC_Data_Types.h
*	@brief Contains widely used data types
*
*	Contains several widely used data types and requests types used internally.
*  
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef TCDATATYPES_H
#define TCDATATYPES_H

#include "TC_Config.h"
#include "TC_Error_Types.h"

/*@}*//**
* @name Networking Data Types
*//*@{*/


/**
* A structure to store a network address (local or remote)
*/
typedef struct net_addr{
	char name_ip[MAX_LOCAL_NAME_SIZE];	/**< The IP address (for remote comunications) or filename (for local comunications) */
	unsigned int port;			/**< The port address number. Must be greater than 0 for remote comunications and equal to 0 for local comunications */
}NET_ADDR;

/**
* The socket types
*/
typedef enum {

LOCAL = 100,		/**< A socket for local comunications */
REMOTE_UDP,		/**< A socket for remote comunications using the UDP protocol */
REMOTE_TCP,		/**< A socket for remote comunications using the TCP protocol */
REMOTE_UDP_GROUP,	/**< A socket for group remote comunications using the UDP protocol (multicast) */

}SOCK_TYPE;

/**
* A structure to store a sockets information
*/
typedef struct sock_entity{

	int fd;		/**< The socket file descriptor number */
	SOCK_TYPE type;	/**< The socket type (LOCAL,REMOTE_UDP,REMOTE_TCP,REMOTE_UDP_GROUP) */

	NET_ADDR host;	/**< The host address to which the socket is bound */
	NET_ADDR peer;	/**< The peer address to which the socket is connected */

}SOCK_ENTITY;
/*@}*/

/*@}*//**
* @name Message Types
*//*@{*/

typedef enum {

REQ_MSG = 1,	/**< Request type message code */
ANS_MSG,	/**< Answer type message code */
DIS_MSG,	/**< Discovery type message code */
EVE_MSG,	/**< Event type message code */

}MSG_TYPE;
/*@}*/


/*@}*//**
* @name Message Operation Types
*//*@{*/

typedef enum {

REG_NODE = 1,	/**< Node registration operation code */
UNREG_NODE,	/**< Node unregistration operation code */
HEART_SIG,	/**< Heartbeat count reset operation code */
REG_TOPIC,	/**< Topic registration operation code */

DEL_TOPIC,	/**< Topic unregistration operation code */
GET_TOPIC_PROP,	/**< Topic get properties operation code */
SET_TOPIC_PROP,	/**< Topic update properties operation code */
REG_PROD,	/**< Topic producer registration operation code */
UNREG_PROD,	/**< Topic producer unregistration operation code */

REG_CONS,	/**< Topic consumer registration operation code */
UNREG_CONS,	/**< Topic consumer unregistration operation code */
BIND_TX,	/**< Topic producer bind operation code */
UNBIND_TX,	/**< Topic producer unbind operation code */
BIND_RX,	/**< Topic consumer bind operation code */
UNBIND_RX,	/**< Topic consumer unbind operation code */

TC_RESERV,	/**< Create topic reservation operation code */
TC_FREE,	/**< Free topic reservation operation code */
TC_MODIFY,	/**< Modify topic reservation operation code */
REQ_ACCEPTED,	/**< Request accepted operation code (for answer type messages only) */
REQ_REFUSED,	/**< Request refused operation code (for answer type messages only) */

}OP_TYPE;
/*@}*/


/*@}*//**
* @name Notification Event Types
*//*@{*/


typedef enum {

EVENT_NODE_PLUG = 1,	/**< Node registration event code */
EVENT_NODE_UNPLUG,	/**< Node unregistration event code */

}EVENT_TYPE;
/*@}*/

/**
* The messages structure for requests and answers
*/
typedef struct net_msg{

/*@}*//**
* @name General Information
*//*@{*/

	MSG_TYPE	type;	/**< The type of message */
	OP_TYPE		op;	/**< The type of operation */
	EVENT_TYPE	event;	/**< The type of event */
	ERR_TYPE	error;	/**< The error code occured while handling a request */
/*@}*/


/*@}*//**
* @name Node Information
*//*@{*/
	unsigned int node_ids[MAX_MULTI_NODES];	/**< The involved nodes ID (I.E for a bind request that requests several nodes to bind at the same time)*/
	unsigned int n_nodes;			/**< The number of involved nodes */
/*@}*/

/*@}*//**
* @name Topic Properties Information
*//*@{*/

	NET_ADDR topic_addr;			/**< The topic network address */			
	unsigned int topic_id;			/**< The ID of the topic */
	unsigned int topic_load;		/**< The topics load/requesting load */
	unsigned int channel_size;		/**< The maximum size of the messages sent through the topic*/
	unsigned int channel_period;		/**< The minimum time interval between consecutive topic messages*/
/*@}*/

}NET_MSG;

/**
* The messages structure for the reservations related requests (used internaly on the client reservation module only)
*/

typedef struct tc_config{

	char operation;		/**< The reseration operation type ('A' -> add/ 'C' -> change/ 'R' -> replace/ 'D' -> delete) */

	char qdisc[20];		/**< Type of desired qdisc (pfifo,htb, etc..) */
	int qdisc_limit;	/**< Qdisc queue size */
	char parent_handle[10];	/**< Parent handle (for queuing/filter if parent_handle string == "root"-> "root" is used in the command instead) */
	char handle[10];	/**< Handle to name qdisc/class/filter */
	char class_id[10];	/**< Handle to identify the class in 'C' operations */
	char flow_id[10];	/**< Handle of the target class (or qdisc) to which a matching filter should send its selected packets */
	char protocol[10];	/**< To be used in filters */
	char dst_ip[20];	/**< Destination IP adress of the packets to be filtered */
	int port;		/**< Destination port adress number of the packets to be filtered */
	int rate;		/**< Hierarchical Token Bucket minimum speed in kbps (i.e 256 kbps) */
	int ceil;		/**< Hierarchical Token Bucket maximum speed in kbps */
	int burst;		/**< Hierarchical Token Bucket burst */
	int cburst;		/**< Hierarchical Token Bucket burst ceil */
	int prio;		/**< Filter priority */	
			
}TC_CONFIG;

/**	
*	@brief Prints the message type code
*
*	Prints a human readable string describing the kind of message from a message type code
*
*	@param[in] type_code	The message type code to be analysed
*
*	@pre			None
*
*	@return			Upon successful return : 0
*	@return			Upon output error : < 0
*/
int tc_msg_type_print ( MSG_TYPE type_code );

/**	
*	@brief Prints the operation message type code
*
*	Prints a human readable string describing the kind of operation from an operation type code
*
*	@param[in] op_code	The operation code to be analysed
*
*	@pre			None
*
*	@return			Upon successful return : 0
*	@return			Upon output error : < 0
*/
int tc_op_type_print ( OP_TYPE op_code );

#endif
