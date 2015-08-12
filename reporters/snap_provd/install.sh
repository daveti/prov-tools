#make clean
make snap_provd

cp snap_provd /usr/bin
setfattr -n security.provenance -v opaque /usr/bin/snap_provd 
chmod +s /usr/bin/snap_provd


killall -9 snap_provd

snap_provd /sys/kernel/debug/provenance0 100000
