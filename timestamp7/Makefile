all: readevents7 

readevents7: readevents7.c timestampcontrol.h configtmst.h
	gcc -Wall -O3 -o readevents7 readevents7.c

readevents5: readevents5.c timestampcontrol.h configtmst.h
	gcc -Wall -O3 -o readevents5 readevents5.c

readevents6: readevents6.c timestampcontrol.h configtmst.h
	gcc -Wall -O3 -o readevents6 readevents6.c

showdiff: showdiff.c
	gcc -Wall -O3 -o showdiff showdiff.c

clean: 
	rm -f *~
	rm -f readevents5
	rm -f readevents6
	rm -f readevents7
	rm -f showdiff

