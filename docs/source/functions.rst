Functions
=========

Here we document the core operating programs and their options used in normal operation. For further documentation of other options, refer to the source code.

chopper
-------
An example::

	chopper  \
		-i $dataroot/rawevents \
		-D $dataroot/sendfiles \
		-d $dataroot/t3 \
		-l $dataroot/t2logpipe \
		-V 4 -U \
		-p $proto \
		-Q 5 -F -y 20 \
		-m $maxeventdiff 2>>$dataroot/choppererror
	
where 

.. program:: chopper

.. option::	-i <infilename>

   filename of source events. Can be a file or a socket which supplies binary data according to the raw data specification from the timestamp unit.
	
.. option::	-D <dir2>
	
	directory in which all type-2 packets are saved. The file names are the epoch in hex (filling zero expanded). Filename is not padded at the end.

.. option::	-d <dir3>

	same as option -D but for type-3 files.

.. option::	-l <logfile>

	For each epoch packet index, an entry is logged into this file. The verbosity level controls the granularity of details logged. If no filename is specified, logging is sent to STDOUT.

.. option::	-V <level>

	Verbosity level control. level is integer, and by default set to 0. The logging verbosity criteria are:
	
	| level<0 : no logging
	| 0 : log only epoc number (in hex)
	| 1 : log epoch, length without text
	| 2 : log epoch, length with text
	| 3 : log epoch, length, bitlength with text
	| 4 : log epoch, detector histos without text

.. option::	-U

	universal epoch; the epoch is not only derived from the timestamp unit digits, but normalized to unix time origin. This needs the timestamp unit to emit event data with an absolute time tag. For this to work, the received data cannot be older than xxx hours, or an unnoted ambiguity error will occur.

.. option::	-p <num>

	select protocol option. defines what transmission protocol is run by selecting what event bits are saved in which stream. option 1 is default.
	
	| 0. service protocol. both type-2 stream and type-3 stream contain the raw detector information.
	| 1. BB84 standard protocol. The type-2 stream contains one bit of basis information, the type-3 stream one bit of value information. The detector sequence is hardcoded in the header.
	| 2. rich BB84. As before, but two  bits are transmitted. if the msb is 0, the lsb has BB84 meaning, if msb is 1, a multi- or no-coincidence event was recorded (lsb=1), or a pair coincidence was detected (lsb=0).
	| 3. six detectors connected to this side, used for the device-independent mode. three transmitted bits, indicating bell basis or key basis
	| 4. four detectors connected to this side, device-independent operation. only time is transmitted.
	| 5. Like 1, but no basis is transmitted, but basis/result kept in local file

.. option::	-Q <num>

	filter time constant for bitlength optimizer. The larger the num, the longer the memory of the filter. for num=0, no change will take place. This is also the default.

.. option::	-F

	flushmode. If set, the logging info is flushed after every write. useful if used for driving the transfer deamon.

.. option::	-y <num>

	Set initial number of events to ignore.

.. option::	-m <maxnum>

  	maximum time for a consecutive event to be meaningful. If the time difference to a previous event exceeds this time, the event is discarded assuming it has to be an error in the timing information. Default set to 0, which corresponds to this option being switched off. Time units is in microseconds.
	
chopper2
--------

An example::

	chopper2 \
		-i $dataroot/rawevents \
		-l $dataroot/t1logpipe \
		-D $dataroot/t1 \
		-V 3 \
		-U -F \
		-m $maxeventdiff 
	
where 

.. program:: chopper2

.. option::	-i <infilename>

	filename of source events. Can be a file or a socket which supplies binary data according to the raw data specification from the timestamp unit.
	
.. option::	-l <logfile>

	For each epoch packet index, an entry is logged into this file. The verbosity level controls the granularity of details logged. If no filename is specified, logging is sent to STDOUT.
	
.. option::	-D <dir1>

	Directory in which all type-1 packets are saved. The file names are the epoch in hex (filling zero expanded). Filename is not padded at the end.

.. option::	-V <level>

	Verbosity level control. level is integer, and by default set to 0. The logging verbosity criteria are:
	
	| level<0 : no logging
	| 0 : log only epoc number (in hex)
	| 1 : log epoch, length without text
	| 2 : log epoch, length with text
	| 3 : log epoch and detailled event numbers for single event counting. format: epoch and 5 cnts spc separated
	
.. option::	-U

	universal epoch; the epoch is not only derived from the timestamp unit digits, but normalized to unix time origin. This needs the timestamp unit to emit event data with an absolute time tag. For this to work, the received data cannot be older than xxx hours, or an unnoted ambiguity error will occur.

.. option::	-F

	flushmode. If set, the logging info is flushed after every write. useful if used for driving the transfer deamon.

.. option::	-m <maxnum>

	Maximum time for a consecutive event to be meaningful. If the time difference to a previous event exceeds this time, the event is discarded assuming it has to be an error in the timing information. Default set to 0, which corresponds to this option being switched off. Time units is in microseconds.


pfind
-----

An example::

	pfind \
		-d $dataroot/receivefiles \
		-D $dataroot/t1 \
		-e $beginepoch \
		-n $useperiods \
		-V 1 \
		-q $akfbufferorder \
		2>>$dataroot/pfinderror
	
where 

.. program:: pfind

.. option::	-d <dir2>

	directory in which all type-2 packets are read from. The file names are the epoch in hex (filling zero expanded). Filename is not padded at the end. These files are the ones transfered over from the other side.

.. option::	-D <dir1>
	
	directory in which all type-1 packets are read from. The file names are the epoch in hex (filling zero expanded). Filename is not padded at the end.
	
.. option::	-e <startepoch>
	
	epoch to start with. Default is 0.

.. option::	-n <epochnums>
	
	define a runtime of epochums epochs before looking for a time delay. default is 1.
	
.. option::	-V <level>
	
	Verbosity level control. level is integer, and by default set to 0. The logging verbosity criteria are:
	
	| -1. : no output
	| 0. : output difference (in plaintext decimal ascii)
	| 1. : output difference and reliability info w/o text
	| 2. : output difference and reliability info with text
	| 3. : more text
								 
.. option::	-q <bufferwidth>
	
	order of FFT buffer size. Defines the wraparound size of the coarse / fine periode finding part. defaults to 17 (128k entries), must lie within 12 and 23.

costream
--------

An example::

	costream \
		-d $dataroot/receivefiles\
		-D $dataroot/t1 \
		-f $dataroot/rawkey \
		-F $dataroot/sendfiles \
		-e $beginepoch \
		-t $timedifference \
		-p $proto \
		-T 2 \
		-m $dataroot/rawpacketindex \
		-M $dataroot/cmdpipe \
		-n $dataroot/genlog \
		-V 5 \
		-G 2 \
		-w $rmtcoinctime \
		-u $trackwindow \
		-Q $tracktime \
		-R 5 \
		-k \
		-K \
		2>>$dataroot/costreamerror

where

.. program:: costream

.. option::	-d <dir2>

	directory in which all type-2 packets are read from. The file names are the epoch in hex (filling zero expanded). Filename is not padded at the end. These files are the ones transfered over from the other side.
	
.. option::	-D <dir1>

	directory in which all type-1 packets are read from. The file names are the epoch in hex (filling zero expanded). Filename is not padded at the end.

.. option::	-f <dir3>

	All type-3 packets are saved into the directory dir3, with the file name being the epoch (filling zero expanded) in hex. Filename is not padded at end. This is the directory with the raw keys.
	
.. option::	-F <dir4>

	All type-4 packets are saved into the directory dir4, with the file name being the epoch (filling zero expanded) in hex. Filename is not padded at end. This is the directory containing the coincidence and basis match info that will be sent to the other side.

.. option::	-e <startepoch>

	Epoch to start with in processing.

.. option::	-t <timediff>
	
	Time difference between the t1 and t2 input streams. This is a mandatory option, and defines the initial time difference between the two local reference clocks in multiples of 125ps.

.. option::	-p <num>
	 
	Protocol index defines the working protocol. Currently implemented:
	
	| 0: service mode, emits all bits into stream 3 locally
	| 1: standard BB84, emits only result in stream 3
	| (2: rich bb84: emits data and base/error info in stream 3)
	| 3: device independent protocol with the 6 detectors connected to the chopper side (low count rate)
	| 4: device independent protocol with the 4 detectors connected to the chopper2 side (high count rate)
	| 5: BC protocol; similar to standard BB84, but handles basis differently.

.. option::	-T <zeropolicy>
	
	Policy how to deal with no valid coincidences in present epoch.
	Implemented:
		
	| 0: do not emit a stream-3 and stream-4 file.
	| 1: only emit a stream-4 file, no stream-3 file to notify the other side to discard the corresp. package. This is the default.
	| 2: emit both stream-3 and stream-4 files and leave the cleanup to a later stage

.. option::	-m <logfile3>
	
	Notification target for type-3 files packets. Locally logged info are epoch numbers in hex form.
	
.. option::	-M <logfile4>
	
	Notification target for type-4 files packets. Logged are epoch numbers in hex form. This file is typically a pipe to notify another process that the type-4 file is ready for processing.
	
.. option::	-n <logfile5>

	Notification target for general information. The logging content is defined by the verbosity level. If no file is specified, or - as a filename, STDOUT is chosen. This file is typically a pipe to another process that displays the information.
	
.. option::	-V <level>

	Verbosity level control. level is integer, and by default set to 0.
	The logging verbosity criteria are:
	
	| level<0 : no output
	| 0 : output bare hex names of processed data sets
	| 1 : output handle and number of key events in this epoch
	| 2 : same as option 1 but with text
	| 3 : output epoch, processed events, sream-4 events, current bit with for stream 4 compression with text
	| 4 : output epoch, processed events, sream-4 events, current bit with for stream 4 compression, servoed time difference,estimated accidental coincidences, and accepted coincidences with text
	| 5 : same as verbo 4, but without any text inbetween

.. option::	-G <flushmode>

	If 0, no fflush takes place after each processed packet
	Different levels:
	
	| 0: no flushing
	| 1: logfile4 gets flushed
	| 2: logfiles for stream3, stream4, standardlog get flushed
	| 3: all logs get flushed

.. option::	-w <window>
	
	coincidence time window in multiples of 1/8 nsec

.. option::	-u <window>
	
	coincidence time window in multiples of 1/8 nsec for tracking shift in the coincidence peak due to clock frequency drifts in the 2 sides.
					
.. option::	-Q <filter>

	filter constant for tracking coincidences. positive numbers refer to events, negative to time constants in microseconds. A value of zero switches tracking off. This is the default.

.. option::	-R <servoconst>

   filter time constant for stream 4 bitlength optimizer. Compression of type 4 files to send to the other side depends on the length. The larger the value, the longer the memory of the filter. for num=0, no change will take place. This is also the default.
						  
.. option::	-H <histoname>

	defines a file containing the histogram of time differences between different detector combinations. If this is empty, no histogram is taken or sent. For a histogram to be prepred the mode of operation must be 0 (service info) to obtain the full 4x4 matrix (or 4x6 for proto3+4).

.. option::	-h

	number of epochs to be included in a histogram file. Default is 10.

.. option::	-k
	
	if set, type-2 input streams are removed after consumption
	
.. option::	-K
	
	if set, type-1 input streams are removed after consumption

splicer
-------

An Example::

	splicer \
		-d $dataroot/t3 \
		-D $dataroot/receivefiles \
		-f $dataroot/rawkey \
		-E $dataroot/splicepipe \
		-p $proto \
		-m $dataroot/genlog \
		-k \
		-K
	
where 

.. program:: splicer

.. option::	-d <dir3>

	All type-3 packets are read from the directory dir3, with the file name being the epoch (filling zero expanded) in hex. Filename is not padded at end.

.. option::	-D <dir4>
	
	All type-4 packets are read from the directory dir4, with the file name being the epoch (filling zero expanded) in hex. Filename is not padded at end. This is the directory containing the coincidence and basis match info that was received from the other side.
	
.. option::	-f <dir3>

	All type-3 sifted key packets are saved into the directory dir3, with the file name being the epoch (filling zero expanded) in hex. Filename is not padded at end. This is the directory with the raw keys.
	
.. option::	-E <cmdpipe>

	This is the pipe which supplies the file (epoch number) of the files in the dir4
	
.. option::	-p <protocol>

	Selection of the protocol type. implemented:
	
	| 0: service mode, emits all bits into stream 3 locally for those entries marked in stream 4
	| 1: selects basebits from stream 3in which are marked in stream4
	| 2: same as mode 0
	| 3: device-independent protocol, this side has 6 detectors
	| 4: device-independent proto, this side has 4 detectors
	| 5: BC version of proto1, just copies received tags from stream 3 into rawkey
	  
.. option::	-m <logfile3>

 	notification target for generated output type-3 packets. Log format is specified by -V option
	
.. option::	-V <level>
	
	Verbosity level control. controls format for logfile in the -m option. level is integer, and by default set to 0. The logging verbosity criteria are:
	
	| level<0 : no output
	| 0 : epoch (in plaintext hex). This is default.

.. option::	-k 

	if set, type-3 input streams are removed after consumption

.. option::	-K
	
	if set, type-4 input streams are removed after consumption
   
	