@echo off
del /S/Q target
rd target
md target

cd src

cl65 -t geos-cbm -O -o ..\..\target\geochess.cvt geochess-res.grc geochess.c

del *.o
del ..\lib\*.o

cd ..\..\target

c1541 -format "geochess,sh" d64 geochess.d64 -write geochess.cvt geochess.cvt -write ../src/geochessfont.cvt geochessfont.cvt

del /Q geochess.cvt

cd ..
