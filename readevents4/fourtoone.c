/* code to convert binary raw data format from readevents4 from output format
   4 to output format 1. Options for absolute timing (-A) and for word 
   swapping (-X) are the same as for readevents4.

   Usage:   fourtoone [-X] [-A] [-Y] [-a outmode]

   takes raw binary data from stdin and process data according to the specified
   output option into a datastream sent to stdout.

*/

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>

#include <unistd.h>
#include <inttypes.h>


#define DEFAULT_OUTMODE 3
#define LOC_BUFSIZ (1<<14)      /* buffer size in 32bit words */

/* some global variables */
int outmode=DEFAULT_OUTMODE;

/* error handling */
char *errormessage[] = {
    "No error.",
    "specified outmode out of range",
    "Can not get absolute time",
};
int emsg(int code) {
    fprintf(stderr,"%s\n",errormessage[code]);
    return code;
};


int main(int argc, char *argv[]){
    int opt; /* option parsing */
    int timemode=0; /* absolute time mode (default: no) */
    int showdummies=0; /* on if dummy events are to be shown on mode -a 1,2 */
    struct timeval timerequest_pointer; /*  structure to hold time request  */
    int swapoption =0; /* for compatibility with old code */
    uint64_t loffset, tl, tl2;
    uint32_t lbuf[LOC_BUFSIZ]; /* input buffer in 32bit */
    uint64_t targetbuf[LOC_BUFSIZ]; /* target buffer */
    int cnt,i,j,k;
    uint32_t tsraw, tsold;
    char c;
    char asciistring[18]; /* keeps empty string */
    

    /* parse option(s) */
    opterr=0; /* be quiet when there are no options */
    while ((opt=getopt(argc, argv, "a:AYX")) != EOF) {
	switch (opt) {
	case 'a': /* set output mode */
	    sscanf(optarg,"%d",&outmode);
	    if ((outmode<0)||(outmode>3)) return -emsg(1);
	    break;
	case 'A': /* absolute time mode */
	    timemode=1;
	    break;
	case 'Y': /* sow dummy events */
	    showdummies=1;
	    break;    
	case 'X': /* swapoption */
	    swapoption=1;
	    break;
	}
    }
    /* prepare buffer pointer for overflow */
    loffset=0LL; tsold=0;

    /* take absolute time if option is set */
    if (timemode) {
	if (gettimeofday(&timerequest_pointer, NULL)) {
	    return -emsg(2);
	}
	loffset = timerequest_pointer.tv_sec; loffset *= 1000000;
	loffset += timerequest_pointer.tv_usec;
	loffset = (loffset*500) << 19;
    }

    /* main parser loop */
    while ((cnt=fread(lbuf,sizeof(uint32_t), LOC_BUFSIZ, stdin))) {
	switch(outmode) {
	case 0: /* output of raw data in hex mode */
	    for (j=0; j<cnt; j++) {
		fprintf(stdout,"%08x\n",lbuf[j]);
	    }
	    break;
	case 1: /* consolidated output in binary mode - fast code */
	     k=0;
	     for (j=0; j<cnt; j++) {
		 tsraw=lbuf[j];
		 if ((tsold ^ tsraw) & 0x80000000) {
		     if ((tsraw & 0x80000000)==0) {
			 loffset += 0x400000000000LL;
		     }
		 }
		 tsold=tsraw;
		 if (tsraw &0x10) {
		     if (!showdummies) continue; /* don't log dummies */
		 }
		 tl = loffset 
		     + ((uint64_t)(tsraw &0xffffffe0)<<14) 
		     + (tsraw & 0x1f);
		 if (swapoption) {
		     tl2=tl<<32L;
		     tl = (tl>>32L) | tl2;
		 }
			targetbuf[k++]=tl;
	     }
	     fwrite(targetbuf, sizeof(int64_t),k,stdout);
	     break;
	case 2: /* consolidated output in hex mode */
	    for (j=0; j<cnt; j++) {
		tsraw=lbuf[j];
		if ((tsold ^ tsraw) & 0x80000000) {
		    if ((tsraw & 0x80000000)==0) {
			loffset += 0x400000000000LL;
		    }
		}
		tsold=tsraw;
		if (tsraw &0x10) {
		    if (!showdummies) continue; /* don't log dummies */
		}
		tl = loffset 
		    + ((uint64_t)(tsraw &0xffffffe0)<<14) 
		    + (tsraw & 0x1f);
		if (swapoption) {
		    tl2=tl<<32L;
		    tl = (tl>>32L) | tl2;
		}
		fprintf(stdout,"%016llx\n",(unsigned long long)tl);
	    }
	    break;
	case 3:
	    /* write stuff as hexdump */
	    j=0;
	    for (i=0; i<cnt; i++) {
		tsraw=lbuf[i];
		for (j=0; j<4; j++) {	
		    c=tsraw & 0xff;
		    fprintf(stdout, "%02x ", c);
		    asciistring[j++] = ((c>31) && (c<127))?c:'.';
		    tsraw >>=8;
		}
		if (i&1) fprintf(stdout, " ");
		if ((i&3)==3) {
		    fprintf(stdout, " %s\n",asciistring);
		    j=0;
		}
	    }
	    break;

	}
    }
    

    return 0;
}


