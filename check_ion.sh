#!/system/bin/sh

if [[ $1 ]]; then
    cat /sys/kernel/debug/ion/$1
else
    ls -l /sys/kernel/debug/ion | head
fi
