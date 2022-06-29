#!/bin/bash

prog_dir=/home/s-fifteen/programs/qcrypto/remotecrypto
local_id=b
remote_id=a

data_dir=$prog_dir/data
pros_data_dir=$data_dir/processed_test
mkdir -p $pros_data_dir/sendfiles $pros_data_dir/t1 $pros_data_dir/t3
send_dir=$pros_data_dir/sendfiles
t1_dir=$pros_data_dir/t1
t3_dir=$pros_data_dir/t3
us=_
pfind_prog=$prog_dir/pfind
pfind_arg="-d $send_dir -D $t1_dir -V 3" #

epoch_list=$pros_data_dir/epoch_list
time_diff_list=$pros_data_dir/pfind_time_diff_list

epoch_count=12
buffer_size=23
fine_res=2
coarse_res=128
skip_epoch=3

##### CHANGE THINGS HERE #####
#begin_epoch='0xb7e82b2b'
begin_epoch='0xb7e89c4c'
##### END CHANGE THINGS ######


echo "$begin_epoch"
use_first_epoch=$(( $begin_epoch + $skip_epoch ))
$pfind_prog $pfind_arg -e $use_first_epoch -n $epoch_count -q $buffer_size -r $fine_res -R $coarse_res 2>outfile #send stderr to outfile to extract timedifference
time_diff=$(cut -f 5 -d' ' outfile | tr -dc '0-9-') # get the time difference from file

rm -f outfile
