#!/bin/sh
# code to generate a data file from the getrate2 samples for gnuplot


#froot=amp-20220819-202111-1- 
#froot=amp-20220819-204909-109- 
froot=amp-20220819-205353-109- 

datadir=data


target=$froot-combine.dat
temp_target0=temp0
temp_target1=temp1
blind_field=3
normal_field=2

# clear target
echo "">$target

for ((i=0; i<6000; i+=100)) do
#for ((i=600; i<1600; i+=1)) do
    # use this if you only want to select one coarse value
    #res=$(awk -f parseonlyone0.awk $datadir/$froot$i.dat )
    # This code should be used when data is generated with readevents5 -a 0
     res=$(cut -f$normal_field --delimiter=" " $datadir/$froot$i.dat | head -n1)
     res1=$(cut -f$blind_field --delimiter=" " $datadir/$froot$i.dat | head -n1)
    echo $i $res >>$temp_target0
    echo $res1 >>$temp_target1
done

paste $temp_target0 $temp_target1 > $target
rm $temp_target0 $temp_target1

