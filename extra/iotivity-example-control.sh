#!/bin/sh
log=/tmp/iotivity-example-control.log
killall -9 client || :

while true ; do \dmesg -c ; sleep .1 ; done | grep --line-buffered  -o '\[R\] KEY_VOLUME.*' | sed -u -n -e 's|.*\[R\] KEY_VOLUME\(.*\)|\1|g;s|.*DOWN.*|2|p;s|.*UP.*|1|p' | /usr/lib/iotivity-example/client | tee "$log"
mv -fv /tmp/iotivity-example-control.log /tmp/iotivity-example-control.log.bak
