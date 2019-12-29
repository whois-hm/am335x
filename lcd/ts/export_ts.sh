#!/bin/sh
export TSDIR=$0
export LD_LIBRARY_PATH=$TSDIR/lib:$LD_LIBRARY_PATH
export TSLIB_TSDEVICE=/dev/input/event1
export TSLIB_PLUGINDIR=$TSDIR/lib/
export TSLIB_PLUGINDIR=$TSDIR/lib/
export TSLIB_PLUGINDIR=$TSDIR/lib/ts/
export TSLIB_CALIBFILE=$TSDIR/etc/pointercal
export TSLIB_FBDEVICE=/dev/fb0

