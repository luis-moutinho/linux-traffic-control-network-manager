#This file is part of LTCNM (Linux Traffic Control Network Manager).
#
#    LTCNM is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    LTCNM is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with LTCNM.  If not, see <http://www.gnu.org/licenses/>.

SRC_PATH=./src

SOCKETS_PATH=$(SRC_PATH)/Utils
TC_CLIENT_PATH=$(SRC_PATH)/Client
TC_SERVER_PATH=$(SRC_PATH)/Server
MISC_PATH=$(SRC_PATH)/Misc

WARNINGS = -Wall

# Include paths

#Utilities include paths
INCLUDES+=-I$(SOCKETS_PATH)
INCLUDES+=-I$(MISC_PATH)

#Client include paths
INCLUDES+=-I$(TC_CLIENT_PATH)/
INCLUDES+=-I$(TC_CLIENT_PATH)/Modules/Monitoring
INCLUDES+=-I$(TC_CLIENT_PATH)/Modules/Database
INCLUDES+=-I$(TC_CLIENT_PATH)/Modules/Reservation
INCLUDES+=-I$(TC_CLIENT_PATH)/Modules/Management
INCLUDES+=-I$(TC_CLIENT_PATH)/Modules/Discovery
INCLUDES+=-I$(TC_CLIENT_PATH)/Modules/Notifications

#Server include paths
INCLUDES+=-I$(TC_SERVER_PATH)/
INCLUDES+=-I$(TC_SERVER_PATH)/Modules/Admission_Control
INCLUDES+=-I$(TC_SERVER_PATH)/Modules/Database
INCLUDES+=-I$(TC_SERVER_PATH)/Modules/Monitoring
INCLUDES+=-I$(TC_SERVER_PATH)/Modules/Management
INCLUDES+=-I$(TC_SERVER_PATH)/Modules/Discovery
INCLUDES+=-I$(TC_SERVER_PATH)/Modules/Notifications

# C++ Compiler settings.
CC=gcc
CCFLAGS=-g -c
CPPFLAGS=$(INCLUDES) $(WARNINGS)

# Linker settings.
LD=$(CC)

#Source files
#Utilities source files
vpath %.c $(SOCKETS_PATH)
vpath %.c $(MISC_PATH)

#Client specific source files
vpath %.c $(TC_CLIENT_PATH)
vpath %.c $(TC_CLIENT_PATH)/Modules/Database
vpath %.c $(TC_CLIENT_PATH)/Modules/Monitoring
vpath %.c $(TC_CLIENT_PATH)/Modules/Reservation
vpath %.c $(TC_CLIENT_PATH)/Modules/Management
vpath %.c $(TC_CLIENT_PATH)/Modules/Discovery
vpath %.c $(TC_CLIENT_PATH)/Modules/Notifications

#Server specific source files
vpath %.c $(TC_SERVER_PATH)
vpath %.c $(TC_SERVER_PATH)/Modules/Database
vpath %.c $(TC_SERVER_PATH)/Modules/Monitoring
vpath %.c $(TC_SERVER_PATH)/Modules/Admission_Control
vpath %.c $(TC_SERVER_PATH)/Modules/Management
vpath %.c $(TC_SERVER_PATH)/Modules/Discovery
vpath %.c $(TC_SERVER_PATH)/Modules/Notifications

CLIENT_SRC_FILES = TC_Client.c TC_Client_DB.c TC_Client_Management.c TC_Client_Monit.c TC_Client_Reserv.c TC_Client_Discovery.c TC_Client_Notifications.c
SERVER_SRC_FILES = TC_Server.c TC_Server_DB.c TC_Server_AC.c TC_Server_Management.c TC_Server_Monitoring.c TC_Server_Discovery.c TC_Server_Notifications.c
SOCKET_SRC_FILES = Sockets.c
UTILS_SRC_FILES	= TC_Utils.c
MISC_SRC_FILES	= TC_Error_Types.c TC_Data_Types.c

OBJ_FILES = $(patsubst %.c, %.o, $(CLIENT_SRC_FILES) $(SERVER_SRC_FILES) $(SOCKET_SRC_FILES) $(UTILS_SRC_FILES) $(MISC_SRC_FILES))

%.o : %.c
	@echo "Compiling $<"
	@$(CC) $(CPPFLAGS) $(CCFLAGS) $< -o $@

all : liblinux_tc.a 
	
liblinux_tc.a: $(OBJ_FILES)
	$(AR) cr $@ $(filter %.o,  $^)
	@cp ./*.a ./libs/
	@cp  $(TC_SERVER_PATH)/TC_Server.h ./includes/
	@cp  $(TC_CLIENT_PATH)/TC_Client.h ./includes/
	@cp  $(MISC_PATH)/TC_Error_Types.h ./includes/	
	@rm -f ./*.[ao]
	
clean :
	@rm -f ./includes/*.h
	@rm -f ./libs/*.a
