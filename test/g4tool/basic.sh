G4TOOL=../../src/g4tool

$G4TOOL -b 2>&1 < ../../testfiles/test.bacon > temp.pbm
md5sum temp.pbm
$G4TOOL -w 600 -h 600 2>&1 < ../../testfiles/rand.600x600.g4 > temp.pbm
md5sum temp.pbm
$G4TOOL -o pcl -b 2>&1 < ../../testfiles/test.bacon > temp.pcl
lpr -Ppcl temp.pcl
md5sum temp.pcl

rm temp.*
