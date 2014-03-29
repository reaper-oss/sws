libebur128
==========

libebur128 is a library that implements the EBU R 128 standard for loudness
normalisation.

All source code is licensed under the MIT license. See LICENSE file for
details.

News
----

The loudness scanning tool has moved to its own repository at
https://github.com/jiixyj/loudness-scanner

Features
--------

* Portable ANSI C code
* Implements M, S and I modes
* Implements loudness range measurement (EBU - TECH 3342)
* True peak scanning
* Supports all samplerates by recalculation of the filter coefficients


Requirements
------------

The library itself has no requirements besides ANSI C.


Installation
------------

In the root folder, type:

    mkdir build
    cd build
    cmake ..
    make

If you want the git version, run simply:

    git clone git://github.com/jiixyj/libebur128.git

Usage
-----

Library usage should be pretty straightforward. All exported symbols are
documented in the ebur128.h header file. For a usage example, see
minimal-example.c in the tests folder.

On some operating systems, static libraries should be compiled as position
independent code. You can enable that by turning on WITH\_STATIC\_PIC.
