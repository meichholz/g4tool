cd testfiles
../tiff2bacon gs_multi_g4.tif
for FILE in 00{0,1,2,3,4,5,6,7,8}
do
  ../g4tool -b <$FILE | xli -zoom 20 stdin
done
rm out.pbm
cd ..

