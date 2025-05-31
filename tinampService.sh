#!/bin/bash
source /etc/profile

type=0
while [ "$type" -eq 0 ]; do
  if pgrep -x "/usr/bin/weston" >/dev/null; then
    type=1
  elif pgrep -x "swaybg" >/dev/null; then
    type=2
  fi
  sleep 0.1
done
if [ "$type" -eq 1 ]; then
  systemctl stop weston
  /usr/bin/weston --flight-rec-scopes= --log=/var/log/weston_tinamp.log -- /roms/ports/Tinamp.sh
  systemctl start weston
elif [ "$type" -eq 2 ]; then
  systemctl stop essway
  /roms/ports/Tinamp.sh
  systemctl start essway
fi
while :; do sleep 10; done
