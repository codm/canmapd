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
CCFLAGS = -pedantic -std=c99 -Wall
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

