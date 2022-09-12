#!/bin/sh

datadir=./data
mkdir -p $datadir

basedate=$(date +%Y%m%d-%H%M%S )
basename=$datadir/amp-den-tmb-$basedate

progflush="../../readevents -q1"
sampleprog="../../readevents -a1 -X -Q -b"
sampleprog2="../../getrate2 -s -n2 -b"
#density=3 #0--7
for ((i=50; i<2851; i+=200)) do 
for ((density=0; density<8; density+=1)) do
for ((timebase=0; timebase<8; timebase+=1))
do
	$progflush >/dev/null
	echo Amp level $i Density level $density Timebase level $timebase
	mode=$((timebase*32 + density*4 + 01))
	const_mode=$(( density*4 + 01))
	seed_arg=$mode,$i,0
	nam=$basename-$i-$density-$timebase.dat
	$sampleprog$seed_arg | $sampleprog2 >$nam
	usleep 100000
done
done
done
