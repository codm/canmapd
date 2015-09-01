##
# cod.m canblocksd makefile
# Author: Tobias Schmitt
# Year: 2015
# (c) cod.m GmbH
#
# Description: Makefile for canblocksd for CAN-Socket
#              Does not support Cross Compiling by now
#
##

NAME = canblocksd
VERSION = 0.1
CCFLAGS = -pedantic -std=c99 -Wall
CC = gcc $(CCFLAGS)
LDFLAGS = -lpthread

OBJ = main.o canblocks.o
all: deploy

debug: CCFLAGS += -DDEBUG -g
debug: deploy

deploy: $(OBJ)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) -c $<

clean:
	rm $(OBJ) $(NAME)

