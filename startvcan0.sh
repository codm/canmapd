#!/bin/sh
sudo modprobe vcan
sudo modprobe can-isotp
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0

