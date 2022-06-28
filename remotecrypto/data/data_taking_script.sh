#!/bin/bash

prog=/home/s-fifteen/programs/qcrypto/timestamp7/readevents5
data_root=/home/s-fifteen/programs/qcrypto/remotecrypto
address='.qkd.internal'
local_id=b
remote_id=a
remote_add=$remote_id$address

local_events=4000000 # 200k per epoch acquire for 20epochs ~ 10s
remote_events=1500000 # 75k per epoch 
arg=" -A -a1 -X -s -Q -q "
mkdir -p $data_root/data/rawevents
file_out_base=$data_root/data/rawevents/raw_
time_list=time_list
us=_

curr_time=$(date  +%Y%m%d%H%M)
echo $remote_add $prog $arg $remote_events > $file_out_base$remote_id$us$curr_time
ssh $remote_add $prog $arg $remote_events > $file_out_base$remote_id$us$curr_time &
$prog $arg $local_events > $file_out_base$local_id$us$curr_time &
echo $curr_time >> $file_out_base$time_list
