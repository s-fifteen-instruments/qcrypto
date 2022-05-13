all: readevents4 showdiff fourtoone readevents4a

readevents4: readevents4.c
	gcc -Wall -O3 -o readevents4 readevents4.c

showdiff: showdiff.c
	gcc -Wall -O3 -o showdiff showdiff.c

fourtoone: fourtoone.c
	gcc -Wall -O3 -o fourtoone fourtoone.c

readevents4a: readevents4a.c
	gcc -Wall -pthread -O3 -o readevents4a readevents4a.c

clean:
	rm readevents4
	rm showdiff
	rm fourtoone
	rm readevents4a
