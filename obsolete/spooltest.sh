#!/bin/sh

echo -n >out
for i in 1 2 3 4 5
do
  echo "testfile number $i"
  g4tool -b -po pcl <testfile >>out
done

lprpages out | lpr -Premote

