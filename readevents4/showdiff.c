/* some test code to show the difference between events. Takes data fro stdin */

#include <stdio.h>
#include <inttypes.h>


#define buflen 1024

int main(int argc, char * argv[]) {
    uint64_t buffer[buflen];
    int n,j;
    uint64_t ts, nanosecs, oldtime, diff, ts2;
    int first=1;
    char state;
    int swapoption=1;

    while ((n=fread(buffer, sizeof(uint64_t), buflen, stdin))) {
	for (j=0; j<n; j++) {
	    ts=buffer[j];
	    if (swapoption) {
		ts2 = ts<<32;
		ts = (ts>>32) |ts2;
	    }
	    nanosecs = (ts>>18) & 0x3ffffffffffeLL;
	    if (first) {
		oldtime = nanosecs; first = 0; continue;
	    }
	    state = ts & 0x1f;
	    diff = nanosecs-oldtime;
	    oldtime = nanosecs;
 
	    printf("%016llx %lld  %01x\n", (long long int) ts, (long long int)diff, state);
	}
    }

    return 0;
}
