#!/bin/bash

if [ "$(basename $(dirname $PWD))" == "tsload" ] &&
	 [ "$(basename $PWD)" == "build" ]; then
	
	if [ $(expr match "$LD_LIBRARY_PATH" .*$PWD.*) -eq 0 ]; then
		export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${PWD}/lib
	fi
	
	if [ $(expr match "$PATH" .*$PWD.*) -eq 0 ]; then
		export PATH=${PATH}:${PWD}/bin
	fi
	
	export TS_MODPATH=${PWD}/mod/
	export TS_LOGFILE=${PWD}/var/run-tsload.log
else
	echo "Should be in tsload/build directory"
fi