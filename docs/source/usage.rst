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

.. c:struct:: header_1 {int tag;
		         unsigned int epoc;
		         unsigned int length;
		         int bitsperentry;
		         int basebits;}



