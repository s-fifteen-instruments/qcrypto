=========
Rationale
=========

  The code is written such that it can operate as asynchronously as possible
  with large amounts of binary data, there should be a minimum of latency
  problems in interprocess communication. In order to allow for modularized
  testing and operation, the code leaves large data in individual files, and
  only exchanges low bandwidth textual control information via pipes. The hope
  is that with modern disk cache techniques, none of the big data files makes
  it ever physically onto the hard disk, because files are removed upon
  consumption by the consuming code. This way, both flexibility in
  reconfiguration and debugging was supposed to be ensured, and in normal
  usage, disk write speed limitation should never be a concern.

  These intermediate files are mostly of binary structure, with a
  header/content structure as described in the :doc:`filespec.txt <file specification>` file.
