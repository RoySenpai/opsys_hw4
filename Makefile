#################################################################################
# 	Operation Systems (OSs) Course Assignment 4 Makefile			#
# 	Authors: Roy Simanovich and Linor Ronen (c) 2023			#
# 	Description: This Makefile compiles the programs and libraries 		#
# 				Date: 2023-05					#
# 			Course: Operating Systems				#
# 				Assignment: 4					#
# 				Compiler: gcc					#
# 				OS: Linux					#
# 			IDE: Visual Studio Code					#
#################################################################################

# Flags for the compiler and linker.
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -g
LIBS = -lpthread
SHAREDLIB = -shared
RM = rm -f

# Phony targets - targets that are not files but commands to be executed by make.
.PHONY: all default clean

# Default target - compile everything and create the executables and libraries.
all: react_server

# Alias for the default target.
default: all


############
# Programs #
############
react_server: react_server.o st_reactor.so
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

##################################
# Libraries and shared libraries #
##################################
st_reactor.so: st_reactor.o
	$(CC) $(CFLAGS) $(SHAREDLIB) -o $@ $^ $(LIBS)

st_reactor.o: st_reactor.c reactor.h
	$(CC) $(CFLAGS) -fPIC -c $<


################
# Object files #
################
%.o: %.c reactor.h
	$(CC) $(CFLAGS) -c $<
	
#################
# Cleanup files #
#################
clean:
	$(RM) *.o *.so react_server