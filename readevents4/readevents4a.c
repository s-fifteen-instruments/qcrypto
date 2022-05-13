/* code to implement new reading code based on posix threads

   reasonably stable code with hardware svn-16    17.12.2016chk
   complete rewrite to work with threads for reading 25.1.2018chk

   usage:
   readevents4a [-U devicefile ]
                [-g gatetime]
	        [-T value]
	        [-a outmode]
	        [-A]
	        [-Y] [-X] [-v]

   options:

   -U devicename      device name of the serial interface; default is 
                      /dev/ttyACM0

   -g gatetime:       sets the gate time in milliseconds; if gatetime==0, the
                      gate never stops. Default is gatetime = 1000msec, the 
		      maximum is 65535.
   -a outmode :       Defines output mode. Defaults currently to 0. Currently
                      implemented output modes are:
		      0: raw event code patterns are delivered as 32 bit
		         hexadecimal patterns (i.e., 8 characters) 
		         separated by newlines. Exact structure of
			 such an event: see card description.
		      1: event patterns with a consolidated timing
		         are given out as 64 bit entities. Format:
			 most significant 45 bits contain timing info
			 in multiples of 2 nsec, least significant
			 4 bits contain the detector pattern.

		      2: output consolidated timing info as 64 bit patterns
		         as in option 1, but as hex text.
		      3: hexdump- like pattern of the input data for debugging
                      4: like 0, but in binary mode. Basically saves raw data
		         from timestamp device.
   -A               sets absolute time mode 

   -T value         sets TTL levels for value !=0, or NIM for value==0 (default)
   -Y               show dummy events. Dummy events allow a timing extension
                    and are emitted by the hardware about every 140 ms if there
		    are no external events.
   -X               exchange 32bit words in timing info for compatibility with
                    old readevents code in output mode 1 and 2...
   -v               increases verbosity of return text

   Signals:
   SIGTERM:   terminates the acquisition process in a graceful way (disabling
              gate, and inhibiting data acquisition.
   SIGPIPE:   terminates the accquisition gracefully without further
              messages on stderr.


*/

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include <unistd.h>
#include <inttypes.h>
#include <errno.h>


#define DEFAULT_GATETIME 0 /* ACQUISITION TIME */
#define NOEVENT_TIMEOUT 200 /* timeout for no events in milliseconds */
#define POLLING_INTERVAL  5 /* read polling in ms */
#define MAX_LOADBYTES 8192   /* maximum bytes read into buffer in a single go */
#define DEFAULTDEVICENAME "/dev/ttyACM0"

#define LOC_BUFSIZ (1<<20)      /* input buffer size in 32bit words */
#define TARGETBUFSIZ 2048       /* target bufffer in 64 bit words */
#define FNAMELENGTH 200      /* length of file name buffers */
#define FNAMFORMAT "%200s"   /* for sscanf of filenames */
#define CMDLENGTH 500        /* command */

#define DEFAULT_OUTMODE 3


/* error handling */
char *errormessage[] = {
    "No error.",
    "Missing command string", /* 1 */
    "Error opening device file",
    "Can not install read thread",
    "Cannot cancel thread", 
    "Output mode 3 obsolete. Use -a 4 option and pipe it into hexdump", /* 5 */
    "timeout while waiting for response",
    "illegal gatetime value",
    "specified outmode out of range",
    "Error reading time",
    "Status read returned unexpected text", /* 10 */
    "Device FIFO overflow",
    "Error or timeout in status read attempt",
};
int emsg(int code) {
    fprintf(stderr,"%s\n",errormessage[code]);
    return code;
};

/* signal handlers */


/* handler for termination */
volatile int terminateflag;
void termsig_handler(int sig) {
    switch (sig) {
	case SIGTERM: case SIGKILL:
	    terminateflag=1;
	    break;
	case SIGPIPE: 
	    /* stop acquisition */
	    terminateflag=1;
    }
}


/* Signal handler and structures for polling mechanism */
struct itimerval newtimer = {{0,0},{0,POLLING_INTERVAL*1000}};
struct itimerval stoptime = {{0,0},{0,0}};

int emptycalls; /* how many timer ticks have happened - gets reset on content */
pthread_mutex_t empty_lock = PTHREAD_MUTEX_INITIALIZER;;  /* count rounds */
int timeerrorflag;

void timer_handler(int sig) {
    if (sig==SIGALRM) {
      if (setitimer(ITIMER_REAL, &newtimer,NULL)) {
	timeerrorflag=1;
      }; /* restart timer */
	/* increase timer calls */
	pthread_mutex_lock( &empty_lock);
	emptycalls++;
	pthread_mutex_unlock( &empty_lock);
    }
}

struct sigaction sigalarm_action = { /* for installing sigalrm handler */
    .sa_flags =  SA_RESTART,
    .sa_handler = timer_handler,
};


/* Reading thread for raw data and communication elements */
uint32_t inbuf[LOC_BUFSIZ];          /* input buffer in 32bit */
char *inbufchar = (char*)inbuf;
volatile int fillidx, fillbytes;     /* buffer status for writing in bytes */
int readerrorflag, readerror;
pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;;  /* for fillbytes */

int fh;  /* device file handle */

void *readthread(void *arg) {
    int cnt, maxbytes, fb;
   
    /* do read, update buffer, and eventually generate an error flag */
    /* only fill to end of buffer, leave remainder to next read */
    //terminateflag=1;
    do {
	//	pthread_mutex_lock(buffer_lock);
	fb=fillbytes; // is there need for atomic access?
	//pthread_mutex_unlock(buffer_lock);

	maxbytes=(LOC_BUFSIZ*sizeof(uint32_t)-fb);
	if (maxbytes>MAX_LOADBYTES) maxbytes=MAX_LOADBYTES;
	if ((maxbytes+fillidx)>(LOC_BUFSIZ*sizeof(uint32_t))) 
	    maxbytes=(LOC_BUFSIZ*sizeof(uint32_t))-fillidx;

	/* do transfer into ring buffer */
	cnt=read(fh, &inbufchar[fillidx], maxbytes);
	if (cnt<0) {
	    readerror=errno; readerrorflag=1; // no need to be atomic
	} else {
	    fillidx=(fillidx+cnt)%(LOC_BUFSIZ*sizeof(uint32_t));

	    pthread_mutex_lock( &buffer_lock);
	    fillbytes += cnt;                 // make this atomic
	    pthread_mutex_unlock(&buffer_lock);

	    pthread_mutex_lock(&empty_lock);
	    if (cnt>0) emptycalls=0;          // make this atomic?
	    pthread_mutex_unlock(&empty_lock);
	}
    } while (!readerrorflag);
    return NULL;
}

int main(int argc, char *argv[]){
    int opt; /* option parsing */
    int outmode=DEFAULT_OUTMODE; /* how to output modes */
    int gatetime = DEFAULT_GATETIME; /* integration time */
    int threshold = 0; /* 0: nim, other value: ttl */
    int verbosity=0; /* for debug texts */
    int timemode=0;  /* absolute time mode */
    int showdummies=0; /* on if dummy events are to be shown on mode -a 1,2 */
    int swapoption =0; /* for compatibility with old code */
    
    char devicefilename[FNAMELENGTH] = DEFAULTDEVICENAME;
    struct termios myterm; /* for configuring terminal input */
    char command[CMDLENGTH];  /* command line to be read in */

    int i;
    int ecdiff; /* emptycall difference for comparison */
    struct timeval timerequest_pointer; /*  structure to hold time request  */
    uint64_t loffset; /* holds absolute time offset for output modes .. */
    uint32_t tsraw, tsold;
    uint64_t tl, tl2;

    int processwords; /* for this round */
    int totalwords ; /* events processed */
    int timeoutflag;
    int drainidx;     /* next read index in input buffer in 32bit words */
    uint64_t targetbuf[TARGETBUFSIZ]; /* for more efficent fwrite */
    int k;

    /* thread-related variables */
    pthread_t rth_descriptor; /* thread handle for read buffering code */
    int retval;

    /* parse option(s) */
    opterr=0; /* be quiet when there are no options */
    while ((opt=getopt(argc, argv, "U:g:a:AT:XYv")) != EOF) {
	switch (opt) {
	case 'U': /* parse device file */
	    if (1!=sscanf(optarg,FNAMFORMAT,devicefilename)) 
		return -emsg(2);
	    break;
	    
	case 'g': /* set the gate time */
	    if (1!=sscanf(optarg,"%i",&gatetime)) return -emsg(2);
	    if ((gatetime<0) || (gatetime>0xffff)) return -emsg(7);
	    break;
	case 'a': /* set output mode */
	    sscanf(optarg,"%d",&outmode);
	    if ((outmode<0)||(outmode>4)) return -emsg(8);
	    if (outmode==3) return -emsg(5);
	    break;
	case 'A': /* absolute time mode */
	    timemode=1;
	    break;
	case 'T': /* set threshold to TTL/NIM */
	    sscanf(optarg,"%d",&threshold);
	    break;
	case 'Y': /* sow dummy events */
	    showdummies=1;
	    break;
	case 'X': /* swapoption */
	    swapoption=1;
	    break;
	case 'v': /* verbosity increase */
	    verbosity++;
	    break;
	}
    }

    /* prepare inbuf */
    for (i=0; i<LOC_BUFSIZ; i+=1024) inbuf[i]=0;
       
    
    /* prepare setup command */
    sprintf(command,"*RST;%s;time %i;timestamp;counts?\r\n",
	    threshold?"TTL":"NIM",
	    gatetime);
    
    /* open device */
    fh=open(devicefilename,O_NOCTTY|O_RDWR);
    if (fh==-1) return -emsg(2);

    /* set terminal parameters */
    cfmakeraw(&myterm);
    myterm.c_iflag = myterm.c_iflag | IGNBRK;
    myterm.c_oflag = (myterm.c_oflag );
    /* mental note to whenever this is read: disabling the CRTSCTS flag
       made the random storage of the send data go away, and transmit
       data after a write immediately.... */
    myterm.c_cflag = (myterm.c_cflag & ~CRTSCTS)|CLOCAL|CREAD;
    myterm.c_cc[VMIN]=1; myterm.c_cc[VTIME]=1;
         
    tcsetattr(fh,TCSANOW, &myterm);
    tcflush(fh,TCIOFLUSH); /* empty buffers */

    loffset=0LL;
    /* send command */
    write(fh,command,strlen(command));

    /* take absolute time if option is set */
    if (timemode) {
	if (gettimeofday(&timerequest_pointer, NULL)) {
	    return -emsg(9);
	}
	loffset = timerequest_pointer.tv_sec; loffset *= 1000000;
	loffset += timerequest_pointer.tv_usec;
	loffset = (loffset*500) << 19;
    }


    /* install termination signal handlers */
    terminateflag=0;
    signal(SIGTERM, &termsig_handler);
    signal(SIGKILL, &termsig_handler);
    signal(SIGPIPE, &termsig_handler);

    /* prepare inpkt flush command */
    strcpy(command,"INPKT\r\n");

    /* prepare input buffer state */
    fillidx=0; drainidx=0; fillbytes=0; readerrorflag=0;
    timeerrorflag=0;

    /* install read buffering thread */
    if (pthread_create(&rth_descriptor, NULL, readthread, NULL)) 
	return -emsg(3);
    
    /* set up timer interval */
    emptycalls=0; timeoutflag=0;
    setitimer(ITIMER_REAL, &newtimer,NULL); // check errors?
    //signal(SIGALRM, &timer_handler);
    sigemptyset(&sigalarm_action.sa_mask);
    sigaction(SIGALRM, &sigalarm_action, NULL);

    /* for processing data */
    totalwords=0; tsold=0;

    /* main loop - does processing */
    do {
	pause();
	if (readerrorflag) {
	    fprintf(stderr, "read error, errno: %d\n",readerror);
	    break;
	}
	if (timeerrorflag) {
	    fprintf(stderr, "read error, errno: %d\n",readerror);
	    break;
	}

	/* check for emptycall situation and eventually flush */
	ecdiff=(NOEVENT_TIMEOUT/POLLING_INTERVAL)-emptycalls;
	if (ecdiff<=0) {
	    if (ecdiff==0) { 
		/* force a flush of the FX2 FIFO with an inpkt command */
		write(fh,command,strlen(command));
	    } else {
		timeoutflag=1;
	    }
	}

	/* read buffer fill state atomically */
	pthread_mutex_lock(&buffer_lock);
	processwords = fillbytes/4;
	pthread_mutex_unlock(&buffer_lock);
	//printf ("read in %d words: \n",processwords);
	
	/* do processing */
	switch(outmode) {
	case 0: /* raw code patterns as hexadecimal patterns (plaintext)*/
	    for (i=0; i<processwords; i++) {
		//tsraw=inbuf[drainidx]; 
		printf("%08x\n", inbuf[drainidx]);
		drainidx=(drainidx+1)%LOC_BUFSIZ;
		totalwords++;
	    }
	    break;
	case 1: /* binary version of consolidated timing */
	    k=0;
	    for (i=0; i<processwords; i++) {
		tsraw=inbuf[drainidx]; //grab raw event for easy access
		if ((tsold ^ tsraw) & 0x80000000) {
		    if ((tsraw & 0x80000000)==0) {
			loffset += 0x400000000000LL;
		    }
		}
		tsold=tsraw; // for overflow comparison in next round
		drainidx=(drainidx+1)%LOC_BUFSIZ;

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

		targetbuf[k++] = tl;
		if (k>=TARGETBUFSIZ) {
		    fwrite(targetbuf,sizeof(int64_t),k,stdout);
		    k=0;
		}
	    }
	    if (k) { // eventually print rest 
		fwrite(targetbuf,sizeof(int64_t),k,stdout);
	    }
	    // TODO: error checking for stdout
	    break;
	case 2: /* consolidated output as hex  text (this is slower ) */
	    for (i=0; i<processwords; i++) {
		tsraw=inbuf[drainidx]; //grab raw event for easy access
		if ((tsold ^ tsraw) & 0x80000000) {
		    if ((tsraw & 0x80000000)==0) {
			loffset += 0x400000000000LL;
		    }
		}
		drainidx=(drainidx+1)%LOC_BUFSIZ;
		tsold=tsraw; // for overflow comparison in next round
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
		fprintf(stdout, "%016llx\n",(long long unsigned int)tl);
		// check for errors?
	    }
	    break;
	case 3: //obsolete - can be replaced
	    break;
	case 4: /* output raw input data into a file */
	    if ((drainidx+processwords)>LOC_BUFSIZ) {
		i=LOC_BUFSIZ-drainidx;
		fwrite(&inbuf[drainidx], sizeof(uint32_t), i, stdout);
		drainidx=0; processwords -=i;
	    }
	    fwrite(&inbuf[drainidx], sizeof(uint32_t),  processwords, stdout);
	    drainidx = (drainidx+processwords)%LOC_BUFSIZ;
	    // do check if write errors?
	    break;
	}
	    
	/* update filling level atomically */
	pthread_mutex_lock(&buffer_lock);
	fillbytes -=sizeof(uint32_t)*processwords;
	pthread_mutex_unlock(&buffer_lock);

    } while (!terminateflag & !timeoutflag);
    
    //fprintf(stderr, "***\n");
    /* close read thread */
    if ((retval=pthread_cancel(rth_descriptor))) {
	printf("errno: %d, ",retval);
	return -emsg(4);
    }
    // Todo: Check if cancel succeeded?
    if ((retval=pthread_join(rth_descriptor,NULL))) {
	printf("join: errno: %d, ",retval);
    }

    /* benignly end the timestamp device and return status */
    strcpy(command, "abort;status?\r\n"); write(fh,command,strlen(command));
    //usleep(10000);

    /* read back response - now in main thread */
    myterm.c_cc[VMIN]=8; myterm.c_cc[VTIME]=2;      
    tcsetattr(fh,TCSANOW, &myterm);
    retval=read(fh, inbufchar, sizeof(inbufchar));
    if ((retval==7)&&(inbufchar[4]==' ')&&(inbufchar[5]=='\r')&&
	(inbufchar[6]=='\n')) {
	i=strtol(inbufchar,NULL,16); // recover value
	if (verbosity>0) fprintf(stderr,"Status word: 0x%04x\n",i);
	if (i&0x10) return -emsg(11); // overflow detected
    } else {
      if (verbosity>0) {
	fprintf(stderr, "Incomplete status return, character cnt: %d\n",retval);
	for (i=0; i<4; i++)
	  fprintf(stderr,"%02x ", inbufchar[i]);
	fprintf(stderr,"\n");
      }
      return -emsg(10);
    }
    strcpy(command, "*rst\r\n"); write(fh,command,strlen(command));
    close(fh);

    return 0;
}
