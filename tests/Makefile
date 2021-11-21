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

TESTS_PATH=./
LIBS_PATH=../libs/
INCLUDES_PATH=../includes/

WARNINGS = -Wall

INCLUDES+=-I$(INCLUDES_PATH)/

# C++ Compiler settings.
CC=gcc
CCFLAGS=-g -c
CPPFLAGS=$(INCLUDES) $(WARNINGS)

# Linker settings.
LD=$(CC)

#Source files
vpath %.c $(TESTS_PATH)
vpath %.a $(LIBS_PATH)

%.o : %.c
	@echo "Compiling $<"
	@$(CC) $(CPPFLAGS) $(CCFLAGS) $< -o $@


all : make_libs TC_API.test

TC_API.test: TC_API.o liblinux_tc.a  
	gcc $(CPPFLAGS) -o $@ $^ -lpthread -lrt -lm
	@rm -f *.o

make_libs: force
	@make -C ../ -s

force: ;

clean :
	@make -C ../ clean -s
	@rm -f *.test


	


