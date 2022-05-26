/* some test code to show the difference between events. Takes data fro stdin */

#include <stdio.h>
#include <inttypes.h>


#define buflen 1024

int main(int argc, char * argv[]) {
    uint64_t buffer[buflen];
    int n,j;
    uint64_t ts, oldtime_n, diff_n, nanosecs, oldtime, diff, ts2, pici;
    int first=1;
    char state;
    int swapoption=0;

    while ((n=fread(buffer, sizeof(uint64_t), buflen, stdin))) {
	for (j=0; j<n; j++) {
	    ts=buffer[j];
	    if (swapoption) {
		ts2 = ts<<32;
		ts = (ts>>32) |ts2;
	    }
	    nanosecs = (ts>>18) & 0x3fffffffffffLL;
	    pici = (ts>>8) & 0x3ffffffffffffcLL;
	    if (first) {
		oldtime_n = nanosecs; first = 0; continue;
		oldtime = pici; first = 0; continue;
	    }
	    state = ts & 0x1f;
	    diff_n = nanosecs-oldtime_n;
	    oldtime_n = nanosecs;
	    diff = pici-oldtime;
	    oldtime = pici;
 
	    printf("%016llx %010lld %07lld %01x\n", (long long int) ts, (long long int)diff, (long long int)diff_n, state);
	}
    }

    return 0;
}
