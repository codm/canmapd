#!/usr/local/bin/python3

# stressTest.py
# Author: Tobias Schmitt
# E-Mail: tschmittldk@gmail.com
#
# This program establishes two connections to open canmapd services which
# have to be connected to the same cansocket device, either virtual or
# over hardware. Then it generates random messages, sends it to one of these
# services and receives it over the other to verify the integrity.
#


import os
import sys
import socket
import time
import random

## init stuff
random.seed()

## Short sumup of functionality:

# TODO: load commandline arguments
# establish connections
try:
    sockIn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sockOut = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
except OSError as msg:
    print(msg)
    sys.exit(1)

try:
    sockIn.connect(("127.0.0.1", 35035))
    sockOut.connect(("127.0.0.1", 35036))
except socket.timeout:
    print("connection could not be established")
    sys.exit(1)

print("connection established... wait for boot")
# spam and receive messages (maybe multithreaded?)
# generate message
time.sleep(1) # give a second to boot
messageCount = 100000
for runs in range(messageCount):
    length = random.randint(32, 64)
    messageString = "00;01;{:04d};".format(length)
    for i in range(0,length):
        rndNum = random.randint(0, 255)
        messageString = messageString + "{:02x}".format(rndNum)
        #messageString = messageString + "ff"
    messageString = messageString + "\n";
    stringlen = 11 + length*2 + 1
    if len(messageString) != stringlen:
        print("message length inconsistent")
    else:
        sockIn.send(messageString)
        response = sockOut.recv(stringlen)
        if response == messageString:
            print("message transferred correctly {:d}".format(length))
        else:
            print("\n\n\nmessage string wrong")
            print("-------ORIGINAL strlen({:d})".format(stringlen))
            print(messageString)
            print("-------RESPONSE strlen({:d})".format(len(response)))
            print(response)
            print("-----------------\n\n\n")
print("finished " + str(messageCount) + " messages")

# cleanup and close
sockIn.close()
sockOut.close()
