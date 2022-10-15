#./st_bin/bash

first_a=37000
block_a=73730
#first_a=73730
#block_a=$first_a
first_a=365000
block_a=204800
first_b=365000
block_b=204800
last=2

a=rawevents/raw_a_
b=rawevents/raw_b_
timestamp=202208110808
out_a=rawevents/st_a_
out_b=rawevents/st_b_
#timestamp=202206291647


count=$first_a
skip=0
input_file=./$a$timestamp
output_file=./$out_a
dd if=$input_file of=$output_file bs=8 count=$count skip=$skip
count=$first_b
skip=0
input_file=./$b$timestamp
output_file=./$out_b
dd if=$input_file of=$output_file bs=8 count=$count skip=$skip

for (( i=1 ; i<=$last ; i++ ))
do
	count=$block_a
	skip=$(( $first_a +($i-1)*$block_a ))
	input_file=./$a$timestamp
	output_file=./$out_a$i
	dd if=$input_file of=$output_file bs=8 count=$count skip=$skip
	count=$block_b
	skip=$(( $first_b +($i-1)*$block_b ))
	input_file=./$b$timestamp
	output_file=./$out_b$i
	dd if=$input_file of=$output_file bs=8 count=$count skip=$skip
done

