Functions
=========

Here we document the core operating programs and their options used in normal operation. For further documentation of options, refer to the source code.

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
	level<0 : no logging
	0. : log only epoc number (in hex)
	1. : log epoch, length without text
	2. : log epoch, length with text
	3. : log epoch, length, bitlength with text
	4. : log epoch, detector histos without text

.. option::	-U

	universal epoch; the epoch is not only derived from the timestamp unit digits, but normalized to unix time origin. This needs the timestamp unit to emit event data with an absolute time tag. For this to work, the received data cannot be older than xxx hours, or an unnoted ambiguity error will occur.

.. option::	-p <num>

	select protocol option. defines what transmission protocol is run by selecting what event bits are saved in which stream. option 1 is default.
	0: service protocol. both type-2 stream and type-3 stream
	contain the raw detector information.
	1: BB84 standard protocol. The type-2 stream contains one bit
	of basis information, the type-3 stream one bit of
	value information. The detector sequence is hardcoded in
	the header.
	2: rich BB84. As before, but two  bits are transmitted. if the
	msb is 0, the lsb has BB84 meaning, if msb is 1, a multi-
	or no-coincidence event was recorded (lsb=1), or a pair
	coincidence was detected (lsb=0).
	3: six detectors connected to this side, used for the
	device-independent mode. three transmitted bits, indicating
	bell basis or key basis
	4: four detectors connected to this side, device-indep
	operation. only time is transmitted.
	5: Like 1, but no basis is transmitted, but basis/result
	kept in local file

.. option::	-Q <num>

	filter time constant for bitlength optimizer. The larger the num, the longer the memory of the filter. for num=0, no change will take place. This is also the default.

.. option::	-F

	flushmode. If set, the logging info is flushed after every write. useful if used for driving the transfer deamon.

.. option::	-y <num>

	Set initial number of events to ignore.

.. option::	-m <maxnum>

  	maximum time for a consecutive event to be meaningful. If the time difference to a previous event exceeds this time, the event is discarded assuming it has to be an error in the timing information. Default set to 0, which corresponds to this option being switched off. Time units is in microseconds.