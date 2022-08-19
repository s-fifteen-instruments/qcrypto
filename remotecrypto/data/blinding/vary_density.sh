#!/bin/sh

datadir=./data

basedate=$(date +%Y%m%d-%H%M%S )
basename=$datadir/den-$basedate

progflush="../../readevents -q1"
sampleprog="../../readevents -a1 -X -Q -b"
sampleprog2="../../getrate2 -s -n2 -b"
timebase=3 #0--7
i=850 
for ((density=0; density<8; density+=1))
do
	$progflush
	mode=$((timebase*32 + density*4 + 01))
	const_mode=$(( timebase*32 + 01))
	seed_arg=$mode,$i,0
	nam=$basename-$const_mode-$i-$density.dat
	$sampleprog$seed_arg | $sampleprog2 >$nam
	usleep 100000
done
