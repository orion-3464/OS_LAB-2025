#!/bin/bash

insmod ./lunix.ko
./mk-lunix-devs.sh
./lunix-attach /dev/ttyS1