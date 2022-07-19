#!/bin/bash

prog_dir=/home/s-fifteen/programs/qcrypto/remotecrypto
local_id=b
remote_id=a

data_dir=$prog_dir/data
pros_data_dir=$data_dir/processed
mkdir -p $pros_data_dir/sendfiles $pros_data_dir/t1 $pros_data_dir/t3
send_dir=$pros_data_dir/sendfiles
t1_dir=$pros_data_dir/t1
t3_dir=$pros_data_dir/t3
raw_base=$data_dir/rawevents/raw_
time_list=time_list
us=_

epoch_list=$pros_data_dir/epoch_list
rm -f $epoch_list
chopper_prog=$prog_dir/chopper
chopper_arg='-V 0 -U -p 0 -Q 5 -F' # verbose 4, epoch universal time, service mode, pack 5, flush output after writing
chopper2_prog=$prog_dir/chopper2
chopper2_arg='-V 0 -U' # verbose 3, epoch universal time
while IFS= read -r line
do
  echo "$raw_base$remote_id$us$line"
  $chopper_prog $chopper_arg -i $raw_base$remote_id$us$line  -D $send_dir -d $t3_dir > outfile 2> /dev/null &
  pid=$! # find chopper pid 
  sleep 8 # let it run for 2 seconds before kill with SIGTERM
  kill $pid
  $chopper2_prog $chopper2_arg -i $raw_base$local_id$us$line -D $t1_dir > outfile2

  read -r ch_first_epoch < outfile
  read -r ch2_first_epoch < outfile2
  echo 'First chopper epoch ' $ch_first_epoch
  echo 'First chopper2 epoch ' $ch2_first_epoch
  if [[ 0x$ch_first_epoch -lt 0x$ch2_first_epoch ]]; then
    begin_epoch=0x$ch2_first_epoch
  else
    begin_epoch=0x$ch_first_epoch
  fi	
  echo $begin_epoch >> $epoch_list
done < "$raw_base$time_list"

rm outfile outfile2

