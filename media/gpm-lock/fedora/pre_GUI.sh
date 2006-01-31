#!/bin/sh

filename="/var/lib/misc/qingy_restart_gpm"
status=`/etc/init.d/gpm status | grep running`

if [ "$status" != "" ]; then

	/etc/init.d/gpm stop >/dev/null 2>/dev/null
	touch $filename

fi
