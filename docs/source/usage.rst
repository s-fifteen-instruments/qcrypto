Usage
=====

.. _installation:

Installation
------------

To use qcrypto, first pull it from github using:

.. code-block:: console

   (.venv) $ git clone https://github.com/s-fifteen-instruments/qcrypto

Operation Modes
---------------

The programs are tailored for various operation modes, namely the service mode
and the key generation mode. In service mode, the full detector information is
transmitted to the other side to allow for source alignment, fiber
neutralization etc. In this mode, the full measurement information is revealed
to the other side, and thus also exposed to an eavesdropper. This is obviously
not secure. The BB84 mode does not transmit this information.

Core Operating Programs
-----------------------

chopper.c: Partitioner and timing data compressor, low count side

  This code digests the raw timestamp output from the readevents program into
  packets of compressed timing/basis information (T2 files) to the other
  side, splices timestamp and CPU timing for long time rollover
  information, and generates local measurement result files for processing
  until the coincidence identification unit returns an acknowledgement. It
  typically operates (and was tested) in a mode where the input comes form a
  named pipe, and the output packets are stored in a directory. Various 
  notification and log information options are given to allow a control
  program to act accordingly upon finished epoch packets. Details of the
  compression in the T2 file spec are in the code directly.

chopper2.c: Partitioner on the high count side

  Similar to chopper, this code partitions raw data generated from the
  readevents program, but does no timing compression, as it is suposed to be
  digested by the costream program directly. Typically it generates T1 files
  into a directory, and notifies a consumer program via a log file or a text
  pipe.

transferd.c: Classical communication gateway

  This code somewhat independently transfers files from various directories
  via a single TCP port to the other machine, and also has some specific pipes
  for GUI and error correction communication in place such that these units
  can communicate independently. The program is used symmetrically on both
  sides.

pfind.c: Initial timing difference finder.

  This code takes some T1 and T2 files, and extracts the initial timing
  difference out of them to initiate the clock tracker in the the costream
  code. AS a status information, it emits also the Signal-to-noise ratio with
  which the peak in the cross correlation function was found. This allows to
  assess the validity of the result. Parameters for this program are the
  number of epochs to be considered, and the depth for the FFT. Typically, the
  FFTs are carried out over 2^17 or 2^19 elements under high background/signal
  conditions. It returns the time difference with an accuracy of 2 nsec. Be
  aware that for proper functionality, the time difference between the two
  sides needs to be better than about 400msec. This accuracy between two host
  computers should be established using an NTP server or similar.

costream.c: Main coincidence identification / sifting code / clock tracking

  This is the main code that compares the basis information from the T2 files with
  the T1 files and identifies the coincidences, and compare the basis depending on
  the mode of operation. For matching basis, the result (raw key) is stored in a T3 file 
  and the basis match identification is saved in a T4 file which will be sent back
  to the low count side.
  
  This code also does clock tracking and offsets small drifts in the time 
  difference between both sides.

splicer.c: carries out the final sifting stage on the low count rate side
	
  With the basis match T4 input file and the T3 input file, this code sifts out
  result for the local and saves it to another T3 file.
  
crgui_ec: TCL/Tk GUI to tie all the codes together.

  This GUI generates the local pipes, files and directories and calls all the 
  programs in the correct sequence in order to start the QKD engine. It also sends
  the appropriate messages to negotiate if the party will be the low or high	count side.

localparams: A complementary file for the gui

  Used for storing individual detector delays, and some default params like
  machine address, target IP etc.

diagnosis.c: for analyzing raw key files in service mode

  This code is used to generate a correlation matrix from t3 files, and just
  counts various detector combination events in that file.

diagbb84.c: Just extracts the length of a bb84 raw key file.

getrate.c: Converts the output from the timestamp unit in rates.

  This code is used for alignment purposes.

Auxillary Processing Code
-------------------------
decompress.c: converts compressed t2 files into hex files

 Used for debugging the chopper algorithm.

ffind.c : Early testing version for the time difference finding program

 uses plain ascii text for timing information. See header for more description.

Documentation Files
-------------------
filespec.txt: Main description of all the binary file formats.

epochdefinition: An incomplete writeup of the timing format used to partition all timing key and communication elements.

gui_spec: An incomplete writeup of communication snippets between the GUI running on both sides

convertdate: A short shell script to convert an epoch index into a human-readable date

convertdate_back: A shell script to convert some time/date string into an epoch


