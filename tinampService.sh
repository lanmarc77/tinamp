#!/bin/bash

#wait for sway to startup
while ! pgrep -x "swaybg" > /dev/null; do
    sleep 1
done
systemctl stop essway
/roms/ports/Tinamp.sh
systemctl start essway
while :; do sleep 10; done
