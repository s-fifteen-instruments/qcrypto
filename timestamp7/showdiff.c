/* some test code to show the difference between events.
 * Prints time, time in ps, ns and us. Takes data fro stdin */

#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>


#define buflen 1024

int main(int argc, char * argv[]) {
    uint64_t buffer[buflen];
    int n,j;
    uint64_t ts, oldtime_n, diff_n, nanosecs, oldtime, diff, ts2, pici;
    uint64_t oldtime_m, diff_m, micro;
    int first=1;
    char state;
    int swapoption=0;
    int opt; /* for parsing command line options */

    /* --------parsing arguments ---------------------------------- */

    opterr=0; /* be quiet when there are no options */
    while ((opt=getopt(argc, argv, "X")) != EOF) {
        switch(opt) {
        case 'X': /* legacy swap option  */
            swapoption=1;
            break;

        }
    }

    while ((n=fread(buffer, sizeof(uint64_t), buflen, stdin))) {
	for (j=0; j<n; j++) {
	    ts=buffer[j];
	    if (swapoption) {
		ts2 = ts<<32;
		ts = (ts>>32) |ts2;
	    }
	    nanosecs = (ts>>18) & 0x3fffffffffffLL;
	    pici = (ts>>8) & 0x3ffffffffffffcLL;
	    micro = (ts>>28) & 0x1fffffffffLL;
	    if (first) {
		oldtime_m = micro; 
		oldtime_n = nanosecs; 
		oldtime = pici; first = 0; continue;
	    }
	    state = ts & 0x1f;
	    diff_m = micro-oldtime_m;
	    oldtime_m = micro;
	    diff_n = nanosecs-oldtime_n;
	    oldtime_n = nanosecs;
	    diff = pici-oldtime;
	    oldtime = pici;
 
	    printf("%016llx %010lld %07lld %04lld %01x\n",
			    (long long int) ts, (long long int)diff,
			    (long long int)diff_n, (long long int)diff_m,
			    state);
	}
    }

    return 0;
}
