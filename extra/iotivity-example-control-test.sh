systemctl stop iotivity-example-control.service ; killall client
log=/tmp/iotivity-example-control-test.log

while true ; do echo '1' ; sleep 2 ; echo 2 ; sleep 2 ; done |  /usr/lib/iotivity-example/client | tee "$log"

