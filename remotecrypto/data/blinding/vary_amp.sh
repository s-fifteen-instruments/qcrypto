#!/bin/sh

datadir=./data

basedate=$(date +%Y%m%d-%H%M%S )
basename=$datadir/amp-$basedate

progflush="../../readevents -q1"
sampleprog="../../readevents -a1 -X -Q -b"
sampleprog2="../../getrate2 -s -n2 -b"
density=3 #0--7
timebase=3 #0--7
for ((i=0; i<6000; i+=100))
do
	$progflush
	mode=$((timebase*32 + density*4 + 01))
	seed_arg=$mode,$i,0
	nam=$basename-$mode-$i.dat
	$sampleprog$seed_arg | $sampleprog2 >$nam
	usleep 100000
done
