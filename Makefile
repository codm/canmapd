##
# cod.m isotpd makefile
# Author: Tobias Schmitt
# Year: 2015
# (c) cod.m GmbH
#
# Description: Makefile for isotpd for CAN-Socket
#              Does not support Cross Compiling by now
#
##

NAME = isotpd
VERSION = 0.1
CCFLAGS = -pedantic -ansi -Wall
CC = gcc $(CCFLAGS)
LDFLAGS =

OBJ = main.o isotpsend.o isotprecv.o isotp.o

all: deploy

debug: CCFLAGS += -DDEBUG -g
debug: deploy

deploy: $(OBJ)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) -c $<

clean:
	rm $(OBJ) $(NAME)


#old debug:
#	gcc -o accessm -g main.c -g isotpsend.c -g isotprecv.c -g isotp.c  -pedantic -ansi -Wall -lpthread

#old deploy:
#	gcc -o accessm main.c isotpsend.c isotprecv.c isotp.c  -pedantic -ansi -Wall -lpthread

