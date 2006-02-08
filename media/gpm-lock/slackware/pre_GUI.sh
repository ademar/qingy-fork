#!/bin/sh

filename="/var/lib/qingy_restart_gpm"
/etc/rc.d/rc.gpm stop >/dev/null 2>/dev/null
status=$?

if [ "$status" == "0" ]; then

	touch $filename

fi
