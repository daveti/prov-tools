#!/bin/bash

make clean
make

cp neo_provd /usr/bin
setfattr -n security.provenance -v opaque /usr/bin/neo_provd 
chmod +s /usr/bin/neo_provd

killall -9 gz_provd

neo_provd /sys/kernel/debug/provenance0 100000