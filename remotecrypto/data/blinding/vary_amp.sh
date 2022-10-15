#!/bin/sh

datadir=./data
mkdir -p $datadir

basedate=$(date +%Y%m%d-%H%M%S )
basename=$datadir/amp-$basedate

progflush="../../readevents -q1"
sampleprog="../../readevents -a1 -X -Q -b"
sampleprog2="../../getrate2 -s -n2 -b"
density=3 #0--7
timebase=3 #0--7
for ((i=0; i<6001; i+=20))
do
	$progflush > /dev/null
	echo Amp level $i
	mode=$((timebase*32 + density*4 + 01))
	seed_arg=$mode,$i,0
	nam=$basename-$mode-$i.dat
	$sampleprog$seed_arg | $sampleprog2 >$nam
	usleep 100000
done
