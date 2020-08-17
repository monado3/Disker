#!/bin/bash
echo '[Server Info]'
uname -n
echo ''

echo '[Kernel Info]'
uname -r
echo ''

echo '[CPU Info]'
lscpu | grep 'Model name'
lscpu | grep 'NUMA node(s)'
lscpu | grep 'Socket(s)'
lscpu | grep 'Core(s)'
lscpu | grep 'Thread(s)'
lscpu | grep '^CPU(s)'
echo ''

echo '[Memory Info]'
echo `awk '( $1 == "MemTotal:" ) { print $2/1048576 }' /proc/meminfo` "GB"
echo ''

echo '[Storage Info]'
lshw | grep -C 6 -6 $1 # $1 = device file
echo ''
