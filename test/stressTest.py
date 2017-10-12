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

print("Hello World")

## Short sumup of functionality:

# load commandline arguments
# establish connections
# spam and receive messages (maybe multithreaded?)
