#!/bin/bash

prog_dir=/home/s-fifteen/programs/qcrypto/remotecrypto
local_id=b
remote_id=a

data_dir=$prog_dir/data
pros_data_dir=$data_dir/processed_test
mkdir -p $pros_data_dir/returnfiles $pros_data_dir/rawkey $pros_data_dir/costream_out
send_dir=$pros_data_dir/sendfiles
t1_dir=$pros_data_dir/t1
t3_dir=$pros_data_dir/t3
returnfiles_dir=$pros_data_dir/returnfiles
rawkey_dir=$pros_data_dir/rawkey
us=_
costream_prog=$prog_dir/costream
costream_arg="-d $send_dir -D $t1_dir -f $rawkey_dir -F $returnfiles_dir -m $pros_data_dir/rawpacketindex -M $pros_data_dir/cmd.pipe -V 5 -p 0 -T 2 -G 2 -w 6 -Q -300000 -u 30 -R 5" #
costream_out=$pros_data_dir/costream_out/costream_out

epoch_list=$pros_data_dir/epoch_list
time_diff_list=$pros_data_dir/pfind_time_diff_list

epoch_count=10
buffer_size=23
fine_res=2
coarse_res=128
skip_epoch=0


##### CHANGE THINGS HERE #####
begin_epoch='0xb7e89c4c'
#begin_epoch='0xb7e85901'
time_diff=497744
#time_diff=-815960
##### END CHANGE THINGS ######
rm -f $costream_out$us$begin_epoch$us'test'
  echo "$begin_epoch  $time_diff"
  use_first_epoch=$(( $begin_epoch + $skip_epoch ))
  $costream_prog $costream_arg -e $use_first_epoch -t $time_diff -n $costream_out$us$begin_epoch$us'test' & # output goes to file
  pid=$!
  sleep 2
  kill $pid
  #time_diff=$(cut -f 5 -d' ' outfile | tr -dc '0-9-') # get the time difference from file
  #echo $begin_epoch $time_diff >> 
  more $costream_out$us$begin_epoch$us'test'
#rm -f outfile

