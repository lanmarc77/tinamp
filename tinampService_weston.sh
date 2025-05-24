#!/bin/bash
source /etc/profile

#systemctl stop weston
while ! pgrep -x "/usr/bin/weston" >/dev/null; do
  sleep 0.1
done
systemctl stop weston
#while ! pgrep -x "/usr/bin/seatd" >/dev/null; do
#  sleep 0.2
#done
#/usr/lib/weston/weston-config
/usr/bin/weston --flight-rec-scopes= --log=/var/log/weston_tinamp.log -- /roms/ports/Tinamp.sh
systemctl start weston
while :; do sleep 10; done
