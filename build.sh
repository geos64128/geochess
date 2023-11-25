#!/bin/sh
rm target/geochess.d64
mkdir target

cd src

cl65 -t geos-cbm -O -o ../target/geochess.cvt geochess-res.grc geochess.c

rm -f *.o

cd ../target

DISK_IMAGE=geochess.d64

c1541 -format "geochess,sh" d64 $DISK_IMAGE \
-write geochess.cvt geochess.cvt \
-write ../src/geochessfont.cvt geochessfont.cvt \

rm -f geochess.cvt

cd ..
