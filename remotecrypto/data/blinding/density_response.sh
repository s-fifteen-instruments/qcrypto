#!/bin/sh
# code to generate a data file from the getrate2 samples for gnuplot


froot=den-20220819-210716-97-850-

datadir=data


target=$froot-combine.dat
temp_target0=temp0
temp_target1=temp1
blind_field=3
normal_field=2

# clear target
echo "">$target

for ((density=0; density<8; density+=1)) do
     res=$(cut -f$normal_field --delimiter=" " $datadir/$froot$density.dat | head -n1)
     res1=$(cut -f$blind_field --delimiter=" " $datadir/$froot$density.dat | head -n1)
    echo $density $res >>$temp_target0
    echo $res1 >>$temp_target1
done

paste $temp_target0 $temp_target1 > $target
rm $temp_target0 $temp_target1

