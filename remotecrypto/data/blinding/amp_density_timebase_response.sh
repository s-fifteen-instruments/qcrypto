#!/bin/sh
# code to generate a data file from the getrate2 samples for gnuplot


froot=amp-den-20220820-101949-97-850-
froot=$(ls -tr data/amp-den-tmb* | tail -n1 | sed  's/data\///' | sed 's/[0-9]*-[0-9]*-[0-9]*.dat$//')


datadir=data
proc_dir=processed
mkdir -p $proc_dir


comb_name=combine.dat
target=$proc_dir/$froot$comb_name
temp_target0=temp0
temp_target1=temp1
blind_field=3,8,9,10,11
normal_field=2,4,5,6,7

# clear target
echo "">$target
for ((i=50; i<2851; i+=200)) do
for ((density=0; density<8; density+=1)) do
for ((timebase=0; timebase<8; timebase+=1)) do
     res=$(cut -f$normal_field --delimiter=" " $datadir/$froot$i-$density-$timebase.dat | head -n1)
     res1=$(cut -f$blind_field --delimiter=" " $datadir/$froot$i-$density-$timebase.dat | head -n1)
    echo $i $density $timebase $res >>$temp_target0
    echo $res1 >>$temp_target1
done

paste --delimiter=" " $temp_target0 $temp_target1 >> $target
rm $temp_target0 $temp_target1
done
done
