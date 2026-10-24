#!/system/bin/sh

netcfg | grep wlan0 | awk '{print $3}'
