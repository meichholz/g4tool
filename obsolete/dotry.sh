#!/bin/sh
i=0
while [ $i -le 350 ]; do
  echo "$i..."
#  ./g4tool -w 2550 -h 3300 < testfiles/005 -v -s $i | xli -zoom 20 stdin
#   ./g4tool -w 2550 -h 3300 < testfiles/xaa.tif -v -s $i | xli -zoom 20 stdin
  skiphead.pl $i <testfiles/xaa.tif >t & viewfax -r -4 -v -w 2550 -h 3300 t
  i=`expr $i + 1`
done
