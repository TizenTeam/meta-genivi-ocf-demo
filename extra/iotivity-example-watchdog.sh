#!/bin/bash -x
log='/tmp/iotivity-example-control.log'
threshold=3
while true ; do
sleep 1
watchdog=${threshold}
[ ! -r "$log" ] || watchdog=$(grep 'resource not yet discovered' "$log" | wc -l || echo $threshold)
if [ $threshold -le ${watchdog} ] ; then
killall client
systemctl stop iotivity-example-control.service ;
rm -fv "$log"
systemctl restart iotivity-example-control.service ;
sleep 5
fi
done
