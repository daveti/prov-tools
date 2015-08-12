make

killall -9 gz_provd
killall -9 snap_provd
killall -9 gz_provd_unzipped

cp gz_provd /usr/bin
setfattr -n security.provenance -v opaque /usr/bin/gz_provd 
chmod +s /usr/bin/gz_provd

gz_provd /sys/kernel/debug/provenance0 /var/log/prov-$(date +%F-%T).log.gz 10000

