cp ../src-orig/*.c .

( cat <<EOM
  int	cxPaper,cyPaper;
  int	xPaperOut;	/* Offset des ersten Ausgabepixels */
  int	cxPaperOut;	/* Größe des Ausgabepuffers */
  int	yPaperOut;	/* das Gleiche für die Höhe */
  int	cyPaperOut;
  int	acRuns[2];
  TRunSpec	arsRuns[2][CRUNTABLE];
  BOOL	        aabitBuffers[Y_LAST_LINE][CX_MAX+5];
  BOOL	        *abitRef,*abitWork; /* only pointer */
  int	        iBuffer;
  unsignedchar	*pchFullPage;
  long		lcchFullPageLine;
  long		lcFullPageLines;

  BOOL		bFullPage;
  BOOL		bLandscape;
  BOOL		bRotate;
  int		nFileFormat;

  int		iLine,iByte;
  int		a,b;

  BOOL		bNoLineWarnings;
  BOOL		bDebugShowRunComp;
  BOOL		bDebugVerbose;
  BOOL		bBacon;
  BOOL		bDebugPCL;
  int		nClipType;
  BOOL		bVerbose;
  BOOL		bDebugEncoder;
  BOOL		bNoCheckEOL;
  int		nPCLResolution;
EOM
) | awk ' /^  / {
	sub(/;.*$/,"",$2);
	split($2,a,/\s*,\s*/);
	for (i in a) {
	var=a[i];
	sub(/^\*/,"",var);
	gsub(/\[.*\]/,"",var);
	print "s/([^A-Za-z_])(" var ")([^\\w_]|$)/$1this->$2$3/g;";
	}
  }' >t

(
for i in CenterPage WriteHeader FlushLine ClosePage \
    EncodePageLine WriteHeadPCL DumpPagePCL FlushLinePCL \
    WriteHeadG4 ClosePageG4 EncodePage FlushLineG4 DecodePage \
    SetTables TiffEnter TiffClose TiffCreate ; do
    echo "s/$i\(/g4t$i\(this,/g;"
done
) >>t

cat t

perl -pi.bak t *.c
rm t


