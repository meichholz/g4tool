for i in hardcore/*.bacon
 do
   echo "processing $i"
   g4tool -b -c -W lines -po pcl <$i >out.pcl
   lprpages out.pcl | lpr -Plj4
 done
 