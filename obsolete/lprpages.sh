#!/bin/sh

########################################################
#
# lprpages {ein} >aus
#
# faßt PCL-Seiten zusammen und sendet sie als Job
#
# GEHÖRT ZU G4TOOL!!!
#
# (C) Marian Eichholz 1997
#
# Version 1.0, 26. 6.1997
#         1.1, 25. 6.1997
#         1.2,  9.10.1998 : Jobframe ist abschaltbar (-n)
#
#########################################################

   VERSION="1.2"

OPT_DUPLEX=0
OPT_OUTBIN=9
OPT_JOBFRAME=YES
OPT_JOBFILE=""

OPTS=1
while [ -n "$OPTS" ]; do
	case "$1" in
	  -D) DEBUG=$2 ; shift 2 ;;
	  -d) OPT_DUPLEX=1 ; shift ;;
	  -O) OPT_OUTBIN="$2"; shift 2 ;;
	  -n) OPT_JOBFRAME=""; shift ;;
	  *) OPTS=""
	esac
done

if [ -z "$1" ]; then
  echo "version: $VERSION"
  echo "  usage: lprpages [-n] [-d] [-D [lj|...]] page1, ..." >&2
  exit 1
fi

#
# Der Mopier scheint folgende Settings nur als DEFAULT zu unterstützen,
# was nicht so toll ist.
#
# Joblokale Settings scheinen nur mit PCL zu funktionieren.
#

# @PJL SET DUPLEX=ON		# ok, DUPLEX geht auch global
# @PJL SET BINDING=LONGEDGE	# dito
# @PJL SET OUTBIN=OUTBIN9	# (das geht dann eben nicht)
# @PJL SET MEDIASOURCE=TRAY3
#

    TRAY=5
  OUTBIN=$OPT_OUTBIN
 ECOMODE=OFF
  DUPLEX=$OPT_DUPLEX
    NAME="\"Dokumente\""
 
 if [ -n "$DEBUG" ]; then
	  OUTBIN=3
	    TRAY=2
	 ECOMODE=ON
	  DUPLEX=0
  if [ $DEBUG = lj ]; then	  
	  OUTBIN=1	# LJ4
	    TRAY=1
  fi	    
 fi

#
# Setze den Drucker auf:
#	Stapler (9G)
#	Tray 3 (4H) oder Tray 4 (5H)
#	Long-Edge-Duplex (1S)
#

echo -en "\033%-12345X"
if [ "$OPT_JOBFRAME" = YES ]; then
  cat <<EOM
@PJL JOB NAME = $NAME 
@PJL RDYMSG DISPLAY = $NAME
@PJL SET ECONOMODE=$ECOMODE
@PJL SET COPIES=1
@PJL ENTER LANGUAGE=PCL
EOM
fi

	#
	# Die Fachanwahl und Duplexoption wird dennoch auf
	# Rahmenebene behandelt!
	#

echo -e -n "\033E\033&l${TRAY}H\033&l${OUTBIN}G\033&l${DUPLEX}S"

# echo -e -n "\033E"

	#
	# Nun werden alle Einzeldokumente zusammengefaßt herausgeworfen
	#
while [ -n "$1" ]; do
  cat $1
  shift 
done

	#
	# Und der Job wird beendet.
	#
if [ "$OPT_JOBFRAME" = YES ]; then
  echo -e -n "\033%-12345X"
  echo "@PJL RDYMSG DISPLAY = \"\""
  echo "@PJL EOJ"
fi 
echo -e -n "\033%-12345X"


