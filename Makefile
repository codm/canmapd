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

NAME = canmapd
VERSION = 0.2
CCFLAGS = -Wall
CC = gcc $(CCFLAGS)
LDFLAGS = -lpthread

OBJ = main.o canmap.o
all: deploy

debug: CCFLAGS += -DDEBUG -g
debug: deploy

deploy: $(OBJ)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) -c $<

clean:
	rm $(OBJ) $(NAME)
