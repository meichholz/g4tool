#!/bin/sh

# $Author: eichholz $
# $Revision: 1.1 $
# $State: Exp $
# $Date: 2001/12/30 10:11:42 $
#
# ############################################################
# g4tool
#
# Umwandlung eines G4-Bildes (ggf. mit Header) in ein PPM
#
# (C) Marian Matthias Eichholz 1. Mai 1997
#
# Benutzung: "dotest <TYP>"
#

TYPE=randprt
#TYPE=recode

CORETEST=farr
CORETEST=dedoc
TESTFILE=testfiles/dedoc

if [ -n "$1" ]
  then TYPE="$1"
fi

case $TYPE in

  core) if [ -n "$2" ]; then CORETEST="$2" ; fi
  	./g4tool -P P150 -P p -bco pcl <testfiles/$CORETEST >out.pcl
	./lprpages -D "" out.pcl | lpr -Premote
	;;

 
  time) time ./g4tool -b <testfile >/dev/null ;;

  neu)
	for i in scans/*.2400x*.g4 ; do
	  ./g4tool -W l -W e -v -h 3496 -w 2400 < $i > out.pbm
	  xli out.pbm
	done
	;;
  rand) ./g4tool -d v -w 600 -h 600 < testfiles/rand.600x600.g4 | xli -zoom 300 stdin
  	;;
  
  randprt) ./g4tool -o p -P 600 -d v -w 600 -h 600 \
  	< testfiles/rand.600x600.g4 \
  	| lpr -Plj4
  	;;
  
  recode) ./g4tool -r -d e -o G4raw -w 600 -h 599 < testfiles/rand.600x600.g4 \
  		> t.g4
  		./g4tool -d v -v -w 599 -h 600 < t.g4 \
  		| xli -zoom 600 stdin
  	;;

  tiff) #./g4tool -d e -o g-tiff -w 600 -h 599 < testfiles/rand.600x600.g4 \
  	./g4tool -o g-tiff -b < hardcore/crash1.bacon \
  		> ~/t.tif
  	;;
  
esac

DEBUG=""

#

#g4tool -b -po pcl <testfile | selp -i 1 | lpr -Plj4

#for i in 1 # 2 3 # 4 5 6 7
#do
#  g4tool -cb -po pcl <testde >out.$i
#done

#xli -zoom 400 testfiles/rand.as*.pbm &


#g4tool -d v -b <hardcore/crash1 >out.pbm 2>t2
#g4tool -W l -b <hardcore/crash1 >out.pbm

#g4tool -v -r -b -d Xencode -o g4 <testfiles/$CORETEST >out.g4
#g4tool -v -w 3508 -h 2592 -d Xv <out.g4 >out.pbm
#g4tool  -v -h 3508 -w 2592 -d Xv <out.g4 >out.pbm
#xli out.pbm &
# $Extended$File$Info$
#
