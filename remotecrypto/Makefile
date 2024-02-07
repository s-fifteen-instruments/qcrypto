all:   chopper chopper2 pfind decompress costream splicer diagnosis transferd  getrate getrate2 diagbb84

chopper: chopper.c
	gcc -Wall -O3 -o chopper chopper.c -lm

chopper2: chopper2.c
	gcc -Wall -O3 -o chopper2 chopper2.c

pfind: pfind.c
	gcc -Wall -O3 -o pfind pfind.c -pthread -lfftw3_threads -lfftw3 -lm

decompress: decompress.c
	gcc -Wall -O3 -o decompress decompress.c

costream: costream.c
	gcc -Wall -O3 -o costream costream.c -lm

splicer: splicer.c
	gcc -Wall -O3 -o splicer splicer.c

diagnosis: diagnosis.c
	gcc -Wall -O3 -o diagnosis diagnosis.c

diagbb84: diagbb84.c
	gcc -Wall -O3 -o diagbb84 diagbb84.c

transferd: transferd.c
	gcc -Wall -O3 -o transferd transferd.c

getrate2: getrate2.c
	gcc -Wall -O3 -o  getrate2 getrate2.c

getrate: getrate.c
	gcc -Wall -O3 -o  getrate getrate.c

# Increase buffer sizes for deployments with high event rates
allow-increased-rates:
	sed -i "s/#define TYPE1_BUFFERSIZE 320/#define TYPE1_BUFFERSIZE 3200/g" chopper2.c
	sed -i "s/#define RAW1_SIZE 640/#define RAW1_SIZE 2000/g" pfind.c
	sed -i "s/#define RAW1_SIZE 640/#define RAW1_SIZE 2000/g" costream.c
	sed -i "s/#define RAW2_SIZE 20/#define RAW2_SIZE 200/g" costream.c
	sed -i "s/#define RAW3_SIZE 150/#define RAW3_SIZE 1500/g" costream.c
	sed -i "s/#define RAW4_SIZE 40/#define RAW4_SIZE 400/g" costream.c
	sed -i "s/LOC_BUFSIZE 1 << 22/LOC_BUFSIZE 1 << 24/g" transferd.c
	sed -i "s/LOC_BUFSIZE2 10000/LOC_BUFSIZE2 100000/g" transferd.c

clean:
	rm -f chopper
	rm -f chopper2
	rm -f pfind
	rm -f decompress
	rm -f costream
	rm -f splicer
	rm -f diagnosis
	rm -f diagbb84
	rm -f transferd
	rm -f getrate
	rm -f getrate2
	rm -f ffind
