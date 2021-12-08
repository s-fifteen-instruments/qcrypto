Prerequisites
=============

This directory contains the core processing programs for a BB84 type key
generation scheme. In order to work properly with the hardware, the user
program for the timestamp card, and a device driver for the appropriate
interface are needed as well. For the USB-interfaced timestamp card, these
directories are  usbtimetagdriver and timestamp3, or the earlier version
timestamp2. For the PCI card based version, a nudaq7200 driver and the
timestamp directory is needed.

The error correction suite can be found in the ec2 directory, also outside
this core directory.

For proper operation of the gui running on TCL/TK, the following links 
from external directories into this one are needed:

A symbolic link to the current readevents program should be set under the name
"readevents". This can be done e.g. with the command
 ln -s ../readevents4/readevents4a ./readevents
The readevents program is the user side part of the interface to talk to the
timestamp card. It tries to efficiently (as in avoidance of memcpy type code)
parse the raw data containing the phase pattern, coarse and fine counter info,
and event register into a single timestamp structure of 64 bit. There is an
option to splice in individual detector delays, and an absolute time
information from the system gettime call. This is the default operation for
the crypto applicatons.

Another symbolic link is needed for getting the error correction deamon
running; target name has to be "errcd". This can be done with the command
 ln -s ../ec2/ecd2 errcd