/* readevents6.c:  Reads in events from the timestamp card 6 and does first
   processing to the data. Configuration happens by commandline options.
   The code is kept partly compatible with the readevents3.c code from the
   qkd timestamp devices from 2003-2008, and uses similar signals / conventions.
   Assumes a pre-loaded phase correction table in the device, but can generate
   simple phase tables as well, or load a LUT from a file.
  
   Copyright (C) 2018-19 Christian Kurtsiefer, National University of Singapore
                      <christian.kurtsiefer@gmail.com> 

 
   output is directed to stdout in a configurable form, and data acquisition
   is controlled via some signals.

   usage:  readevents6 [-q maxevents] [-r | -R ] [-v verbosity]
                       [-a outmode] [-A] 
		       [-U devicenode]
		       [-X ]
		       [-Q ]
		       [-c  coincvalue ]
		       [-P polarity ]
		       [-j nimvalue ] [-d]
		       [-t threshold ] [-T threshold]
		       [-u] [-F]

		       
   -q maxevents :     quit after a number of maxevents detected events.
                      Default is 0, indicating eternal operation.
   -r :               starts immediate acquisition after card setup.
                      This is default.
   -R :               Initializes card, but does not start acquisition
                      until  a SIGUSR1 is received.
   -a outmode :       Defines output mode. Defaults currently to 0. Currently
                      implemented output modes are:
		      0: raw event code patterns are delivered as 64 bit
		         hexadecimal patterns (i.e., 16 characters) 
		         separated by newlines. Exact structure of
			 such an event: see card description.
		      1: event patterns with a consolidated timing
		         are given out as 64 bit entities. Format:
			 most significant 54 bits contain timing info
			 in multiples of 1/256 nsec, least significant
			 4 bits contain the detector pattern.
		      2: output consolidated timing info as 64 bit patterns
		         as in option 1, but as hext text.
   -v verbosity :     selects how much noise is generated on nonstandard
                      events. All comments go to stderr. A value of 0
		      means no comments. Default is 0.
   -A :		      Output in absolute time. The timestamp mark is added to
                      the unix time evaluated upon starting of the timestamp
		      card; the resulting time is truncated to the least
		      significant 54 bit in multiples of 1/256 nsec.
   -U devicename:     allows to draw the raw data from the named device node.
                      If not specified, the default is /dev/ioboards/timestamp0
   -L lookuptabname:  define a file that contains a lookup table (plus ADC
                      preprocess info) instead of using the linear fill and
		      lores routing.
   -X                 Legacy swap option for high word / low word
   -Q                 Power off after termination
   -P polarity:       Select polarity. 0: negative (NIM), 1: positive (TTL)
   -c coincvalue:     Set the coincidence time window in internal FPGA delay
                      steps. Range is 0...31, default is ??
   -j nimvalue:       if !=0 overrides the nim divider and auxline setting
   -d                 switch on debug mode bit
   -t threshold:      sets pos or neg thresold in dac values (-512...511)
                      according to polarity for both polarities. Overrides
		      defaults from flash
   -T threshold:      sets pos or neg threshold in millivolt (-1800...1800).
                      Overrides defaults from flash.
   -u                 flushes USB transfer every 10ms
   -F                 flushes text output with every line (binary unaffected)
		      
   Sigsnals:
   SIGUSR1:   enable data acquisition. This causes the inhibit flag
              to be cleared, and data flow to start or to be resumed.
   SIGUSR2:   stop data acquisition. leads to a setting of the inhibit flag.
              Data in the FIFO and the DMA buffer will still be processed.
   SIGTERM:   terminates the acquisition process in a graceful way (disabling
              DMA, and inhibiting data acquisition into the hardware FIFO.
   SIGPIPE:   terminates the accquisition gracefully without further
              messages on stderr.


    Status of code:
     First code draft 8.6.2018 chk
     Basic functionality in place 19.6.2018 chk
     added bootup delay for PLL to stabilize 1.5.2019chk

    ToDo:

    - proper table initialization ok?
    - proper output format handling - modes 0 and 2 seem to work
    - 32 bit intermediate format processing


*/

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>


#include "timestampcontrol.h"
#include "configtmst.h"


#define FILENAMLEN 200
#define DEFAULTDEVICENAME "/dev/ioboards/usbtmst0"

#define DEFAULT_POLLING_INTERVAL 10 /* in milliseconds */
#define DEFAULT_OUTMODE 0
#define DEFAULT_VERBOSITY 0
#define DEFAULT_MAXEVENTS 0
#define DEFAULT_COLLECTIONMODE 1 /* for -r/-R option */
#define DEFAULT_COINC 0  /* for -c option */
#define MAX_COINC_VALUE 31

#define DAC0DEFAULT 900 /* for neg polarity */
#define DAC1DEFAULT 300 /* for pos polarity */

/* content of the readback byte from GET_POWER_STATE*/
#define FPGA_booted 0x80
#define Powerline 0x20
#define LUT_LOADED 0x01

/* readback buffer stuff. These numbers are in bytes */
#define Readback_buffersize (1<<22)
#define INITIAL_TRANSFERLENGTH (1<<16)
#define dmasize_in_uint64 (Readback_buffersize / sizeof(uint64_t))

/* some global variables */
int outmode = DEFAULT_OUTMODE;
int verbosity = DEFAULT_VERBOSITY;
int collectionmode = DEFAULT_COLLECTIONMODE; /* 0: don't start, 1: do start */

uint32_t *rbbuffer; /* pointer to readback buffer */

/* things needed for the USB device  */
#define default_usbtimetag_devicename "/dev/ioboards/timestamp0"
char usbtimetag_devicename[200]= default_usbtimetag_devicename; 

/* ----------------------------------------------------------------*/
/* error handling */
char *errormessage[] = {
    "No error.",
    "Cannot open device.", /* 1 */
    "Cannot parse device file name",
    "error parsing verbosity level",
    "Error reading power status",
    "Error configuring device", /* 5 */
    "Error resetting device",
    "Error parsing option",
    "Error parsing lookup table filename",
    "Cannot open lookup file",
    "Lookuptable does not contain enough rows", /* 10 */
    "Lookuptable contains illegal entry",
    "Error parsing outmode",
    "outmode out of range (0..2)",
    "Can not parse maxevents",
    "Maxevents out of range", /* 15 */
    "Error reading LUT from flash",
    "LUT loading error",
    "Error reading scratch RAM",
    "Error writing scratch RAM",
    "Error transferring LUT to FPGA", /* 20 */
    "Error writing config",
    "Error writing ADC parameters",
    "Error starting USB transfer on host side",
    "Cannot start Streaming in FX2",
    "Cannot stop FX2 streaming", /* 25*/
    "Cannot stop USB transfer on host side",
    "Error configuring clock chip",
    "Error configuring ADC chip",
    "Error installing signal handler",
    "Error in getting number of bytes in main loop", /* 30 */
    "Error in processing raw data",
    "Error getting system time",
    "Error reading coincidence value",
    "Coincidence value out of range (0...31)",
    "Error parsing nim value", /* 35 */
    "Error writing DAC levels",
    "Error parsing polarity parameter",
    "Error parsing threshold",
    "Threshold out of range (-512...511 DAC units)",
    "Threshold out of range (-1800...1800 mV)", /* 40 */
};
int emsg(int code) {
  fprintf(stderr,"%s\n",errormessage[code]);
  return code;
};

/* ----------------------------------------------------------------------*/
/* signal handler */
/* structures for timer interrupts */
const struct itimerval polltime = {{0,DEFAULT_POLLING_INTERVAL*1000},
				   {0,DEFAULT_POLLING_INTERVAL*1000}};
const struct itimerval stoptime = {{0,0},{0,0}}; 
void timer_handler(int sig) {
    /* nothing to do, we just return */
}
/* declare handler */
struct sigaction timeraction = {.sa_handler = &timer_handler,
				.sa_flags = SA_RESTART,};

/* SIGUSR machinery */
int sigusrnote=0; /* to notify main loop about a signal, avoid complicated
		   stuff at this point, as sigusr are not very time-critical */
void sigusr_handler(int sig) {
    if (sig==SIGUSR1) sigusrnote=1;
    if (sig==SIGUSR2) sigusrnote=2;
}
/* declare handler */
struct sigaction sigusr_action = {.sa_handler = &sigusr_handler,
				  .sa_flags = SA_RESTART,};
/* termination signal handler */
int running=0; /* global process variables for running status */
void sigterm_handler(int sig) {
    switch (sig) {
    case SIGTERM: case SIGPIPE:
	running=0;
	break;
    }
}
/* declare handler */
struct sigaction sigterm_action = {.sa_handler = &sigterm_handler,
				   .sa_flags = SA_RESTART,};


/* variables and code for processing events */
int numberofsamples = DEFAULT_MAXEVENTS; /* sampled events. 0 means unlimited,
					int is ok for small limited lengths */
int processedevents = 0; /* only relevant if numberofsamples !=0 */
uint64_t offsettime[16]; /* time differences for different detector states */
int legacyswapoption=0;  /* if set, swap to old version of binary output */
int flushmode=0;         /* text flush option */

/* intermediate buffer for processed events */
uint64_t outbuf[dmasize_in_uint64]; /* holds postprocessed data  */


/* code to process timestamp data into an output stream. Returns number of
   processed 32bit words, or a negative number on error or exception */
int process_data(uint32_t *rbbuffer, int startindex, int endindex, 
		 FILE *outfile) {
    int i,j, j2, endindex2;
    uint64_t rawevent;
    uint32_t highword, lowword;

    switch (outmode) {
    case -1: /* raw binary output */
    case 0:  /* direct data, conversion to hex in 32bit chunks - legacy mode */
	endindex2=endindex;
	if (numberofsamples) { /* calculate how many words we can process */
	    if ((2*(endindex-startindex)) > (numberofsamples-processedevents)) {
		endindex2=startindex+(numberofsamples-processedevents)*2;
		running=0; /* eventually stop acquisition */
	    }
	}
	if (outmode) { /* send out raw binary data */
	    fwrite(&rbbuffer[startindex], sizeof(int32_t),
		   endindex2-startindex, outfile);
	} else {      /* print raw data in hex format as 32bit chunks */
	    for (i=startindex; i<endindex2; i++) 
		fprintf(outfile, "%08x\n",rbbuffer[i]);
	    if (flushmode) fflush(outfile);
	}
	return endindex2-startindex;

    case 1: /* postprocessed output, eventually with a word swap for legacy
	       software compatibility - binary version  */
    case 2: /* same as mode 1, but hex version */
	/* todo : honor maxevents */
	j=0;
	for (i=startindex; i+1<endindex; i+=2) {
	    rawevent = rbbuffer[i] | (((uint64_t)rbbuffer[i+1])<<32);
	    /* do any consistency checks etc here */
	    if (rawevent==0) continue; /* we have a void urb return */
	    /* do any time corrections here */
	    outbuf[j] = rawevent + offsettime[rawevent&0xf]; j++;
	    /* do event number test */
	    if (numberofsamples) {
		processedevents++;
		if (processedevents >= numberofsamples) {
		  running=0;break;
		}
	    }
	}

	/* do 32bit word swap for legacy output if needed */

	/* now we need to output this */
	if (outmode==1) { /* this is plain binary output */
	    if (legacyswapoption) { /* swap first and second 32bit words */
		for (j2=0; j2<j; j2++) {
		    highword = outbuf[j2]>>32LL; lowword=outbuf[j2]&0xffffffff;
		    outbuf[j2]= (uint64_t)lowword<<32 | highword;
		}
	    }
	    fwrite(outbuf, sizeof(uint64_t), j, outfile);
	} else { /* this is the hex text version */
	    for (j2=0; j2<j; j2++) {
	      fprintf(outfile, "%016llx\n",
		      (long long unsigned int) outbuf[j2]);
	    }
	    if (flushmode) fflush(outfile);
	}
	return 2*j; /* number of processed uint32 entries */
	
    }

    /* this should never be reached */
    return -31; /* something silly happened */
}


int main(int argc, char *argv[]) {
    int opt; /* for parsing command line options */
    int handle; /* file handle for usb device */
    int configword = 0;   /* holds the default config register in FPGA of
			     the timestamp card */
    char devicefilename[FILENAMLEN] = DEFAULTDEVICENAME;
    char lookupfilename[FILENAMLEN] = "";
    int retval;
    int powerstat; /* variable for power status */
    int lookuptable[2048];
    FILE *lookupfile;
    int i;
    uint16_t checksum, scratchram[128]; /* scratch ram mirror */
    int sendvalue, sendvalue2;
    int v, vold;  /* number of bytes acquired so far, mod 2^31 */
    long long vll, v2gb; /* extension for >2GByte processed data */
    int looperror; /* for read in loop */
    unsigned long long prev_processed_bytes, tmp, tmp2;
    int startindex, bytesforthisround;
    int absolutetimemode=0;
    struct timeval systemtimestamp; /*  structure to hold abs time request */
    uint64_t absolutetime; /* holds absolute time */
    int poweroffmode=0;
    int polarity=0;
    int coinc_value = DEFAULT_COINC;
    int nimvalue=0; // temporary hack
    int debugmode=0; // to set debug bit
    int threshold=0; /* 0 marks no parameter was set */
    unsigned int dacword=0; /* combined threshold words for DAC */
    int USBflushmode=0; /* to flush USB transfer every 10 ms */

    /* --------parsing arguments ---------------------------------- */
    
    opterr=0; /* be quiet when there are no options */
    while ((opt=getopt(argc, argv, "U:v:q:a:rRAXQPc:j:dt:T:uF")) != EOF) {
	switch(opt) {
	case 'q': /* set number of samples to be read in */
	    if (sscanf(optarg,"%d", &numberofsamples)!=1 ) return -emsg(7);
	    break;
	case 'U': /* enter device file name */
	    if (sscanf(optarg,"%99s",devicefilename)!=1 ) return -emsg(2);
	    devicefilename[FILENAMLEN-1]=0; /* close string */
	    break;
	case 'v': /* set verbosity level */
	    if (1!=sscanf(optarg,"%d",&verbosity)) return -emsg(3);
	    break;
	case 'L': /* get a lookup table file name */
	    if (sscanf(optarg, "%200s", lookupfilename)!=1) return -emsg(8);
	    lookupfilename[FILENAMLEN-1]=0; /* safety termination */
	    break;
	case 'a': /* choose outmode */
	    if (sscanf(optarg, "%d", &outmode)!=1) return -emsg(12);
	    if ((outmode<0) || (outmode>2)) return -emsg(13);
	    break;
	case 'r': /* begin immediately with acquisition */
	    collectionmode=1;
	    break;
	case 'R': /* wait acquisition until signal comes */
	    collectionmode=0;
	    break;
	case 'A': /* set absolute time mode */
	    absolutetimemode=1;
	    break;
	case 'X': /* legacy swap option for binary output in mode 2 */
	    legacyswapoption=1;
	    break;
	case 'Q': /* power off mode */
	    poweroffmode=1;
	    break;
	case 'P': /* select polarity */
	    if (1!=sscanf(optarg,"%d",&polarity)) return -emsg(37);
	    break;
	case 'c': /* set coincidence delay value */
	    if (1!=sscanf(optarg,"%d",&coinc_value)) return -emsg(33);
	    if ((coinc_value<0) || (coinc_value>31)) return -emsg(34);
	    break;
	case 'j': // nim value hack
	    if (1!=sscanf(optarg,"%d",&nimvalue)) return -emsg(35);
	    break;
	case 'd': /* debug mode on */
	    debugmode=1;
	    break;
	case 't': /* set threshold in dac units */
	case 'T': /* set threshold in millivolt */
	    if (1!=sscanf(optarg,"%d",&threshold)) return -emsg(38);
	    if (opt=='t') {
		if ((threshold<-512) ||(threshold>511)) return-emsg(39);
	    } else {
		if ((threshold<-1800) ||(threshold>1796)) return-emsg(40);
		threshold=threshold*512/1800; /* convert to DAC units */
	    }
	    dacword=threshold&0x3ff; /* truncate to 10 bit */
	    dacword = (dacword | dacword<<16); /* both words */
	    threshold=1; /* mark valid threshold in dacword */
	    break;
	case 'u': /* USB flush mode */
	    USBflushmode=1;
	    break;
	case 'F': /* text flash mode */
	    flushmode=1;
	    break;
	}
    }

    /* install signal handlers: polling timer, user signals */
    if (sigaction(SIGALRM, &timeraction, NULL)) return -emsg(29);
    if (sigaction(SIGUSR1, &sigusr_action, NULL)) return -emsg(29);
    if (sigaction(SIGUSR2, &sigusr_action, NULL)) return -emsg(29);
    if (sigaction(SIGTERM, &sigterm_action, NULL)) return -emsg(29);
    if (sigaction(SIGPIPE, &sigterm_action, NULL)) return -emsg(29);
        
    /* open device */
    handle=open(devicefilename,O_RDWR | O_NONBLOCK);
    if (handle==-1) {
      fprintf(stderr, "errno: %d; ",errno);
      return -emsg(1);
    }

    /* ------------- initialize hardware  ---------------------*/
    /* eventually stop running acquisition */
    if (verbosity>2)
	fprintf(stderr, "Stopping previous acquisition (FX2 part)....");
    if (ioctl(handle, RESET_TRANSFER)) return -emsg(6);
    if (verbosity>2) fprintf(stderr, "OK\n");

    /* find out about power state */
    if (ioctl(handle, GET_POWER_STATE, &powerstat)) return -emsg(4);

    /* turn device on, boot FPGA, initialize clock and ADC */
    if (ioctl(handle, CONFIG_TMSTDEVICE, 2)) return -emsg(5);
    
    /* allow device to boot and PLL to stabilize */
    if ((powerstat &(Powerline | FPGA_booted)) != (Powerline | FPGA_booted)) {
      do {
	if (ioctl(handle, GET_POWER_STATE, &powerstat)) return -emsg(4);
	usleep(150000); // allow PLL to stabilize for 150ms after FPGA is up
      } while ((powerstat &(Powerline | FPGA_booted)) !=
	       (Powerline | FPGA_booted));
    }

    /* eventually stop running acquisition */
    if (verbosity>2)
	fprintf(stderr, "Stopping previous acquisition (device driver)....");
    if (ioctl(handle, STOP_STREAM)) return -emsg(6);
    if (verbosity>2) fprintf(stderr, "OK\n");


    /* Check if we need to load the LUT table from a file */
    if (lookupfilename[0]) {  
      if (verbosity>2)
	fprintf(stderr, "Reading lookup table....");
      lookupfile=fopen(lookupfilename, "r");
      if (!lookupfile) return -emsg(9);
      for (i=0; i<2048; i++) {
	if (1!=fscanf(lookupfile,"%x",&lookuptable[i])) return -emsg(10);
	if (lookuptable[i] & 0xffff000f) return -emsg(11);
      }
      if (verbosity>2)
	fprintf(stderr, "OK\n");
	 
    } else { /* simple linear population as a default */
	for (i=0; i<2048; i++) lookuptable[i]=i<<4;
    }
        
    /* some non-time-critical variables to set */
    absolutetime=0;

    /* allocate and map I/O memory */
    rbbuffer =  (uint32_t *) mmap(NULL, Readback_buffersize,
				  PROT_READ|PROT_WRITE,
				  MAP_SHARED, handle, 0);
    if (rbbuffer == MAP_FAILED) return -emsg(5);
    /* pre-populate page tables by visiting each of them */
    for (i=0; i< (Readback_buffersize/4); i+=1024) retval=retval+rbbuffer[i];
    if (verbosity>2) fprintf(stderr, "Memory buffer prepared\n");

    /* Check if LUT is loaded in timestamp FX2 RAM */
    if (ioctl(handle, GET_POWER_STATE, &powerstat)) return -emsg(4);
    if (!(powerstat & LUT_LOADED)) {
	/* try to reload LUT */
    	if(ioctl(handle, RETREIVE_EEPROM)) return -emsg(16);
    	if (ioctl(handle, GET_POWER_STATE, &powerstat)) return -emsg(4);
    	if (!(powerstat & LUT_LOADED)) return -emsg(17);
    }

    /* do consistency check if flash data is ok. This means that
       1. number of data words (ex count and chksum) in the first 2 bytes
       2. an offset/routing in the second word
       3. Scratch ram contains a valid checksum (all add up to 0).
       A valid sequence is 0x0001 0x0011 0xffee  */
    for (checksum=0,i=0; i<128; i++) {
    	retval=4096+2*i;
    	if (ioctl(handle, READ_RAM, &retval)) return -emsg(18);
    	scratchram[i]=retval>>16;
    	if (verbosity>3) fprintf(stderr,"i: %02x: %04x\n",i,scratchram[i]);
    	checksum += scratchram[i];
    	if (i>scratchram[0]) break;
    	}
    if (verbosity>3) fprintf(stderr,"checksum: %d\n",checksum);

    /* if checksum==0 here, we have a valid flash entry */
    sendvalue2=0; /* default: no change of DAC thresholds */
    if (checksum != 0) { /* invalid chksum, fill FX RAM with LUT data*/
    	for (i=0; i<2048; i++) {
    	    sendvalue=((lookuptable[i]&0xffff)<<16) | (i<<1);
    	    if (ioctl(handle, WRITE_RAM, &sendvalue)) return -emsg(19);
    	}
	sendvalue = 0; /* no adc offset, lores routing 2us periode for NIM */
    } else { /* we have a valid checksum, compose a config that honors
		both simple and NIM config info */
	sendvalue = scratchram[1] | ((scratchram[0]>1?scratchram[2]:0)<<16);
	if (scratchram[0]>2) sendvalue2=scratchram[3]; /* DACVAL0 for neg */
	if (scratchram[0]>3) 
	    sendvalue2 |= (scratchram[4]<<16); /* DACVAL1 for pos */
    }
    if (verbosity>2) fprintf(stderr, "LUT checked in eeprom\n");

    // temporary hack
    if (nimvalue) {
      sendvalue = (sendvalue & 0xffff) | (nimvalue <<16);
    }
    /* eventually override DAC setting from Flash */
    if (threshold) sendvalue2=dacword;
	
    /* send configuration */
    configword = (coinc_value<<10) | LongFormat | NoDummyInject | NIMOUTenable ;
    if (debugmode) configword |= TimestampDebug;
    if (polarity) configword |= PositiveInputPolarity; /* eventually set pol */

    //printf("configword: %04x\n",configword);

    if (ioctl(handle, WRITE_CPLD, configword | ParameterSelect | CounterReset))
      return -emsg(21);
    /* send ADC and NIM parameters */
    if (ioctl(handle, WRITE_CPLD_LONG, sendvalue)) return -emsg(22);
    if (sendvalue2) {
	if (ioctl(handle, WRITE_CPLD_LONG, sendvalue2)) return -emsg(36);
    }
    if (verbosity>2) fprintf(stderr,"OK\n");
    // quick hack: Set ADC levels
    //if (ioctl(handle, WRITE_CPLD_LONG, (dacval1<<16) | dacval0)) return -emsg(22);

    /* send LUT from FX2 RAM to FPGA */
    if (ioctl(handle, WRITE_CPLD, configword | LookuptabSelect| CounterReset))
       return -emsg(21);
    if (verbosity>2) fprintf(stderr,"Pushing LUT to FPGA.....");
    if (ioctl(handle, PUSH_LOOKUP)) return -emsg(20);
    if (verbosity>2) fprintf(stderr, "OK\n");


    /* Reset counters and FIFos on card */
    sendvalue = FIFOreset | CounterReset | configword; /* acquisition on */
    if (ioctl(handle, WRITE_CPLD, sendvalue)) return -emsg(21);

    /* start host side USB engine */
    if (ioctl(handle, Start_USB_machine))return -emsg(23);
    
    /* unreset counter and fifo in FPGA, switch on collection */
    sendvalue = configword;
    if (sigusrnote) {
	collectionmode = (sigusrnote==1)?1:0; sigusrnote=0;
    }
    /* get absolute time */
    if (gettimeofday(&systemtimestamp, NULL)) return -emsg(32);
    
    if (collectionmode) sendvalue |= CollectEN|CollectLED; /* acquisition on */
    if (ioctl(handle, WRITE_CPLD, sendvalue)) return -emsg(21);

    /* start streaming data from the  fx2 */
    if (verbosity>2) fprintf(stderr, "Starting acquisition in FX2..." );
    if (ioctl(handle, START_STREAM)) return -emsg(24);
    if (verbosity>2) fprintf(stderr, "OK\n" );
    
    /* now sort out time offsets */
    if (absolutetimemode) {
	absolutetime=((uint64_t)systemtimestamp.tv_sec)*1000000LL
	    + (uint64_t)systemtimestamp.tv_usec;
	absolutetime = (absolutetime*1000LL)<<18; /* 10 bit info, 8bit to nsec */
    }
    /* set time offsets for different detectors */
    for (i=0; i<16; i++) offsettime[i]= absolutetime;

    setitimer(ITIMER_REAL, &polltime, NULL); /* initiate polling timer */
    
    running=1; looperror=0; prev_processed_bytes=0; vold=0;
    v2gb=0;

    /* ------------- start acquisition - main loop ---------*/
    do {
	pause(); /* see polltime structure for typical duration */

	/* check if we need to update the acquisition status */
	if (sigusrnote) {
	    sendvalue = configword;
	    collectionmode=(sigusrnote==1)?1:0; sigusrnote=0;
	    if (collectionmode) sendvalue |= CollectEN|CollectLED; /* acq on */
	    if (ioctl(handle, STOP_STREAM)) {looperror=25; continue;}
	    if (ioctl(handle, WRITE_CPLD, sendvalue)) {looperror=21; continue;} 
	    if (ioctl(handle, START_STREAM)) {looperror=24;continue;}
	}
	v=ioctl(handle, Get_transferredbytes);

	if (verbosity>5) fprintf(stderr, "bytes: %d\n",v);

	if (v<0) { /* check for errors */
	    looperror=30;
	    break;
	}

	if (USBflushmode) { /* check if we need to flush the usb pipeline */
	    if (v==vold) { /* this is a flush condition */
		if (ioctl(handle, FLUSH_FIFO)) looperror=41;
		continue;
	    }
	}
	/* do 2GB extension, limited by readback value from transfer */
	if (v<vold) v2gb += 0x80000000L;
	vold=v;
	
	/* check how many bytes to postprocess this round; avoid rollover */
	vll=v+v2gb;
	tmp=vll/Readback_buffersize; 
	tmp2=prev_processed_bytes/Readback_buffersize;
	if (tmp > tmp2)
	    vll=(tmp2+1)*Readback_buffersize; /* only ascending buffers */
	/* vll now contains the number of bytes (from start) to be 
	   processed in this round */
	
	bytesforthisround = (vll-prev_processed_bytes) & ~7LL;
	startindex = (prev_processed_bytes % Readback_buffersize)
	    /sizeof(uint32_t);
	
	/* do actual processing of one buffer segment */
	retval = process_data(rbbuffer,
			      startindex, startindex + bytesforthisround/4,
			      stdout);
	
	if (retval<0) looperror=-retval;
	
	prev_processed_bytes += bytesforthisround;
    } while (running && !looperror);
    
    /* ----- the end ---------- */

    sendvalue =  configword; /* acquisition off */
    if (ioctl(handle, WRITE_CPLD, sendvalue)) return -emsg(21);
    /* stop streaming */
    if(ioctl(handle, STOP_STREAM) ) return -emsg(25);
    sendvalue = FIFOreset | configword;
    if (ioctl(handle, WRITE_CPLD, sendvalue) ) return -emsg(21);
    /* stop hostside USB engine */
    if (ioctl(handle, Stop_USB_machine)) return -emsg(26);
    
    /* switch off timer */
    setitimer(ITIMER_REAL, &stoptime, NULL); 

    /* power off mode */
    if (poweroffmode) {
	if (ioctl(handle, CONFIG_TMSTDEVICE, 0)) return -emsg(5);
    }
    /* error messages */
    if (looperror) return -emsg(looperror);
    
    return 0;
}
