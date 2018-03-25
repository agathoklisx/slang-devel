#!/bin/sh
gcc tcc-module.c \
  -I/usr/local/include -g -O2 -Wl,-R/usr/local/lib \
   --shared -fPIC -ltcc -lslang \
  -o tcc-module.so
