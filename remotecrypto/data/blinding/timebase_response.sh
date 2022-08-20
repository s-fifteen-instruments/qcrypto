#!/bin/sh
# code to generate a data file from the getrate2 samples for gnuplot


froot=tmb-20220820-102006-13-850-

datadir=data
proc_dir=processed
mkdir -p $proc_dir

comb_name=combine.dat
target=$proc_dir/$froot$comb_name
temp_target0=temp0
temp_target1=temp1
blind_field=3
normal_field=2

# clear target
echo "">$target

for ((timebase=0; timebase<8; timebase+=1)) do
     res=$(cut -f$normal_field --delimiter=" " $datadir/$froot$timebase.dat | head -n1)
     res1=$(cut -f$blind_field --delimiter=" " $datadir/$froot$timebase.dat | head -n1)
    echo $timebase $res >>$temp_target0
    echo $res1 >>$temp_target1
done

paste $temp_target0 $temp_target1 > $target
rm $temp_target0 $temp_target1

