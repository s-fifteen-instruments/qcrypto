Usage
=====

.. _installation:

Installation
------------

To use qcrypto, first pull it from github using:

.. code-block:: console

   (.venv) $ git clone https://github.com/s-fifteen-instruments/qcrypto

Creating recipes
----------------

The data structures of packets in communitcations are:

.. c:struct:: header_1 

   .. c:var:: int tag
   .. c:var:: unsigned int epoc
   .. c:var:: unsigned int length
   .. c:var:: int bitsperentry
   .. c:var:: int basebits

