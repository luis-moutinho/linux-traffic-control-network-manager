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

/**	@file TC_Config.h
*	@brief Contains several configuration and debug parameters
*
*	Contains several configuration parameters. Most of these values are optimized and should not be changed.
*	Debug messages in each of the client and server modules can be activated here
*  
*	@author Luis Silva (luis.silva.ua@gmail.com)
*	@bug No known bugs
*	@date 31/12/2012
*/

#ifndef TCCONFIG_H
#define TCCONFIG_H


/*@}*//**
* @name Structural Parameters
*//*@{*/


/**	@def MAX_LOCAL_NAME_SIZE
*	@brief Maximum local name string length
*/
#define MAX_LOCAL_NAME_SIZE 100

/**	@def MAX_IP_SIZE
*	@brief Maximum IP address string length
*/
#define MAX_IP_SIZE 20

/**	@def MAX_MULTI_NODES
*	@brief Maximum number of simultaneous changes in nodes (I.E. maximum of simultaneous unbinds)
*/
#define MAX_MULTI_NODES 10

/**	@def D_MTU
*	@brief Maximum UDP to split data messages into fragments ( (sizeof(IP Header) + sizeof(UDP Header) + SEQ_N + DATA_SIZE) = 65535-(20+8+4+4) = 65499 )
*/
#define D_MTU 65499
/*@}*/


/*@}*//**
* @name Reservations Parameters
*//*@{*/


/**	@def RESERV_SLACK_MULTIPLIER
*	@brief Used to give a slack to the topic reservations made. This value is multiplied with the topic load value. The result is the total
*	reserved bandwidth. (I.E. 1.04 -- 4% more bandwidth -- should be the minimum value to account for the IP+UDP headers) 
*/
#define RESERV_SLACK_MULTIPLIER 1.04

/**	@def PFIFO_SIZE
*	@brief Reservation FIFOs size (number of packets) -- This can actually limit the max size of messages (need ~45 for one UDP with size 65499)
*/
#define PFIFO_SIZE 150

/**	@def ROOT_BW
*	@brief Maximum NIC bandwidth (in mbps) to be used for comunications+control
*/
#define ROOT_BW 100

/**	@def BACKGROUND_BW
*	@brief Maximum bandwidth (in mbps) to be used for background traffic
*/
#define BACKGROUND_BW 1

/**	@def CONTROL_BW
*	@brief Maximum root bandwidth (in mbps) to be used for control traffic (I.E. Heartbeats, requests)
*/
#define CONTROL_BW 25

/**	@def MAX_USABLE_BW
*	@brief Maximum bandwidth (in bps) to be used by apps
*/
#define MAX_USABLE_BW ((ROOT_BW-BACKGROUND_BW-CONTROL_BW)*1024*1024)
/*@}*/


/*@}*//**
* @name General Timeout Parameters
*//*@{*/


/**	@def DISCOVERY_TIMEOUT
*	@brief Maximum time interval (in ms) waiting for for discovering a server
*/
#define DISCOVERY_TIMEOUT 5000

/**	@def FRAG_TIMEOUT
*	@brief Maximum time interval (in ms) waiting for arrival of a message fragment
*/
#define FRAG_TIMEOUT 600

/**	@def C_REQUESTS_TIMEOUT
*	@brief Maximum time interval (in ms) to receive the answer of client request from server
*/
#define C_REQUESTS_TIMEOUT 100

/**	@def S_REQUESTS_TIMEOUT
*	@brief Maximum time interval (in ms) to receive the answer of server request from client
*/
#define S_REQUESTS_TIMEOUT 100

/**	@def BIND_LOCK_TIMEOUT
*	@brief Maximum time interval (in ms) to lock to a topic send/receive mutex
*/
#define BIND_LOCK_TIMEOUT 50

/**	@def UNBIND_TIMEOUT
*	@brief Maximum time interval (in ms) to be unbound
*/
#define UNBIND_TIMEOUT 50
/*@}*/


/*@}*//**
* @name Control Periodicity Parameters
*//*@{*/


/**	@def HEARTBEAT_GEN_PERIOD
*	@brief Periodicity (in us) for the generation of heartbeats by clients
*/
#define HEARTBEAT_GEN_PERIOD 100000

/**	@def HEARTBEAT_DEC_PERIOD
*	@brief Periodicity (in us) for the decrementation by the server of the clients heartbeat count
*/
#define HEARTBEAT_DEC_PERIOD 100000

/**	@def HEARBEAT_COUNT
*	@brief Number of consecutive periods without receiving an heartbeat from a client before it is declared dead
*/
#define HEARBEAT_COUNT 5

/**	@def DISCOVERY_GEN_PERIOD
*	@brief Periodicity (in us) for the generation of discovery messages by server
*/
#define DISCOVERY_GEN_PERIOD 150000
/*@}*/


/*@}*//**
* @name Control Remote Addresses
*//*@{*/


/**	@def MONITORING_PORT_OFFSET
*	@brief Monitoring modules port address offset from the server requests port
*/
#define MONITORING_PORT_OFFSET 1

/**	@def RESERVATION_PORT_OFFSET
*	@brief Reservation modules port address offset from the server requests port
*/
#define RESERVATION_PORT_OFFSET 2

/**	@def MANAGEMENT_PORT_OFFSET
*	@brief Management modules port address offset from the server requests port
*/
#define MANAGEMENT_PORT_OFFSET 3

/**	@def DISCOVERY_GROUP_PORT
*	@brief Discovery group messages port number
*/
#define DISCOVERY_GROUP_PORT 5000

/**	@def DISCOVERY_GROUP_IP
*	@brief Discovery group messages IP address
*/
#define DISCOVERY_GROUP_IP "239.200.200.200"

/**	@def NOTIFICATIONS_GROUP_PORT
*	@brief Notifications group messages port number
*/
#define NOTIFICATIONS_GROUP_PORT 5001

/**	@def NOTIFICATIONS_GROUP_IP
*	@brief Notifications group messages IP address
*/
#define NOTIFICATIONS_GROUP_IP "239.200.200.200"

/**	@def MANAGEMENT_GROUP_PORT
*	@brief Management group messages port number
*/
#define MANAGEMENT_GROUP_PORT 5002

/**	@def MANAGEMENT_GROUP_IP
*	@brief Management group messages IP address
*/
#define MANAGEMENT_GROUP_IP "239.200.200.200"
/*@}*/


/*@}*//**
* @name Control Local Addresses
*//*@{*/

/**	@def SERVER_AC_LOCAL_FILE
*	@brief Servers admission control module local filename
*/
#define SERVER_AC_LOCAL_FILE			"server_ac_local"

/**	@def SERVER_MONITORING_LOCAL_FILE
*	@brief Servers monitoring module local filename
*/
#define SERVER_MONITORING_LOCAL_FILE		"server_monitoring_local"

/**	@def SERVER_RESERVATION_LOCAL_FILE
*	@brief Servers reservation module local filename
*/
#define SERVER_RESERVATION_LOCAL_FILE		"server_reservation_local"

/**	@def SERVER_MANAGEMENT_REQ_LOCAL_FILE
*	@brief Servers management module local filename (to send requests)
*/
#define SERVER_MANAGEMENT_REQ_LOCAL_FILE 	"server_management_req_local"

/**	@def SERVER_MANAGEMENT_ANS_LOCAL_FILE
*	@brief Servers management module local filename (to receive answers from the client node)
*/
#define SERVER_MANAGEMENT_ANS_LOCAL_FILE 	"server_management_ans_local"

/**	@def SERVER_DISCOVERY_LOCAL_FILE
*	@brief Servers discovery module local filename
*/
#define SERVER_DISCOVERY_LOCAL_FILE		"server_discovery_local"

/**	@def SERVER_NOTIFICATIONS_LOCAL_FILE
*	@brief Servers notifications module local filename
*/
#define SERVER_NOTIFICATIONS_LOCAL_FILE		"server_notifications_local"

/**	@def CLIENT_AC_LOCAL_FILE
*	@brief Clients request module local filename
*/
#define CLIENT_AC_LOCAL_FILE			"client_ac_local"

/**	@def CLIENT_MONITORING_LOCAL_FILE
*	@brief Clients monitoring module local filename
*/
#define CLIENT_MONITORING_LOCAL_FILE		"client_monitoring_local"

/**	@def CLIENT_RESERVATION_LOCAL_FILE
*	@brief Clients reservation module local filename
*/
#define CLIENT_RESERVATION_LOCAL_FILE		"client_reservation_local"

/**	@def CLIENT_MANAGEMENT_REQ_LOCAL_FILE
*	@brief Clients management module local filename (to receive requests from the server)
*/
#define CLIENT_MANAGEMENT_REQ_LOCAL_FILE 	"client_management_req_local"

/**	@def CLIENT_MANAGEMENT_ANS_LOCAL_FILE
*	@brief Clients management module local filename (to send answers)
*/
#define CLIENT_MANAGEMENT_ANS_LOCAL_FILE 	"client_management_ans_local"

/**	@def CLIENT_DISCOVERY_LOCAL_FILE
*	@brief Clients discovery module local filename
*/
#define CLIENT_DISCOVERY_LOCAL_FILE		"client_discovery_local"

/**	@def CLIENT_NOTIFICATIONS_LOCAL_FILE
*	@brief Clients notifications module local filename
*/
#define CLIENT_NOTIFICATIONS_LOCAL_FILE		"client_notifications_local"
/*@}*/


/*@}*//**
* @name Debug Parameters
*//*@{*/


/**	@def ENABLE_DEBUG_SOCKET
*	@brief If 1 enables debug messages at the socket module. If 0 disables them
*/
#define ENABLE_DEBUG_SOCKET 0

/**	@def ENABLE_DEBUG_UTILS
*	@brief If 1 enables debug messages at the utils module. If 0 disables them
*/
#define ENABLE_DEBUG_UTILS 0

/**	@def ENABLE_DEBUG_CLIENT
*	@brief If 1 enables debug messages at the client top module. If 0 disables them
*/
#define ENABLE_DEBUG_CLIENT 0

/**	@def ENABLE_DEBUG_CLIENT_MONIT
*	@brief If 1 enables debug messages at the client monitoring module. If 0 disables them
*/
#define ENABLE_DEBUG_CLIENT_MONIT 0

/**	@def ENABLE_DEBUG_CLIENT_DB
*	@brief If 1 enables debug messages at the client database module. If 0 disables them
*/
#define ENABLE_DEBUG_CLIENT_DB 0

/**	@def ENABLE_DEBUG_CLIENT_MANAGEMENT
*	@brief If 1 enables debug messages at the client management module. If 0 disables them
*/
#define ENABLE_DEBUG_CLIENT_MANAGEMENT 0

/**	@def ENABLE_DEBUG_CLIENT_RESERV
*	@brief If 1 enables debug messages at the client reservation module. If 0 disables them
*/
#define ENABLE_DEBUG_CLIENT_RESERV 0

/**	@def ENABLE_DEBUG_CLIENT_DISCOVERY
*	@brief If 1 enables debug messages at the client discovery module. If 0 disables them
*/
#define ENABLE_DEBUG_CLIENT_DISCOVERY 0

/**	@def ENABLE_DEBUG_CLIENT_NOTIFICATIONS
*	@brief If 1 enables debug messages at the client notifications module. If 0 disables them
*/
#define ENABLE_DEBUG_CLIENT_NOTIFICATIONS 0

/**	@def ENABLE_DEBUG_CLIENT_DB
*	@brief If 1 enables debug messages at the client database module. If 0 disables them
*/
#define ENABLE_DEBUG_CLIENT_DB 0

/**	@def ENABLE_DEBUG_SERVER
*	@brief If 1 enables debug messages at the server top module. If 0 disables them
*/
#define ENABLE_DEBUG_SERVER 0

/**	@def ENABLE_DEBUG_SERVER_AC
*	@brief If 1 enables debug messages at the server admission control module. If 0 disables them
*/
#define ENABLE_DEBUG_SERVER_AC 0

/**	@def ENABLE_DEBUG_SERVER_DB
*	@brief If 1 enables debug messages at the server database module. If 0 disables them
*/
#define ENABLE_DEBUG_SERVER_DB 0

/**	@def ENABLE_DEBUG_SERVER_TOPIC_DB
*	@brief If 1 enables debug messages at the server database topic submodule. If 0 disables them
*/
#define ENABLE_DEBUG_SERVER_TOPIC_DB 0

/**	@def ENABLE_DEBUG_SERVER_NODE_DB
*	@brief If 1 enables debug messages at the server database node submodule. If 0 disables them
*/
#define ENABLE_DEBUG_SERVER_NODE_DB 0

/**	@def ENABLE_DEBUG_SERVER_MANAGEMENT
*	@brief If 1 enables debug messages at the server management module. If 0 disables them
*/
#define ENABLE_DEBUG_SERVER_MANAGEMENT 0

/**	@def ENABLE_DEBUG_SERVER_MONIT
*	@brief If 1 enables debug messages at the server monitoring module. If 0 disables them
*/
#define ENABLE_DEBUG_SERVER_MONIT 0

/**	@def ENABLE_DEBUG_SERVER_RESERV
*	@brief If 1 enables debug messages at the server reservation module. If 0 disables them
*/
#define ENABLE_DEBUG_SERVER_RESERV 0

/**	@def ENABLE_DEBUG_SERVER_DISCOVERY
*	@brief If 1 enables debug messages at the server discovery module. If 0 disables them
*/
#define ENABLE_DEBUG_SERVER_DISCOVERY 0

/**	@def ENABLE_DEBUG_SERVER_NOTIFICATIONS
*	@brief If 1 enables debug messages at the server notifications module. If 0 disables them
*/
#define ENABLE_DEBUG_SERVER_NOTIFICATIONS 0
/*@}*/

#endif
