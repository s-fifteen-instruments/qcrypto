#!/bin/sh
# code to generate a data file from the getrate2 samples for gnuplot


froot=den-20220820-101949-97-850-
froot=$(ls -tr data/den* | tail -n1 | sed  's/data\///' | sed 's/[0-9]*.dat$//')


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

for ((density=0; density<8; density+=1)) do
     res=$(cut -f$normal_field --delimiter=" " $datadir/$froot$density.dat | head -n1)
     res1=$(cut -f$blind_field --delimiter=" " $datadir/$froot$density.dat | head -n1)
    echo $density $res >>$temp_target0
    echo $res1 >>$temp_target1
done

paste --delimiter=" " $temp_target0 $temp_target1 > $target
rm $temp_target0 $temp_target1

