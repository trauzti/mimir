#!/bin/sh
autoreconf -fis
./configure
sed -i "s/O2/O3/g" Makefile
make

