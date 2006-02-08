#!/bin/sh

filename="/var/lib/qingy_restart_gpm"

if [ -f $filename ]; then

	/etc/rc.d/rc.gpm start >/dev/null 2>/dev/null
	rm $filename

fi
