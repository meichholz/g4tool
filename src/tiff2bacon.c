/*RCS-Head: $Id: tiff2bacon.c,v 1.1 2001/12/30 10:11:58 eichholz Exp $ */

/**

<tiff2bacon.c>

tiff2bacon

Das Programm splittet ein Multipage-G4-TIFF auf mehrere BACON-geheaderte
Roh-G4-Files auf.

Es ist *kein* Universalprogramm!

Es basiert auf dem <tiffsplit.c> von Sam Leffler aus der <libtiff>

 * Copyright (c) 1992-1996 Sam Leffler
 * Copyright (c) 1992-1996 Silicon Graphics, Inc.

(c) Marian Eichholz at DuSPAT 15.3.1999

Es ist wirklich eher ein Down-Strip von Sams Programm, ok?

Versionen:
0.19 : 15.03.1999 (startup)
0.20 : 16.03.1999 : Bitorder-Problem gefixt, BACON-Schmalspurheader.
0.21 :              Verbosings wieder raus.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tiffio.h"

#define	streq(a,b)	(strcmp(a,b) == 0)
#define	CopyField(tag, v) \
    if (TIFFGetField(in, tag, &v)) TIFFSetField(out, tag, v)
#define	CopyField2(tag, v1, v2) \
    if (TIFFGetField(in, tag, &v1, &v2)) TIFFSetField(out, tag, v1, v2)
#define	CopyField3(tag, v1, v2, v3) \
    if (TIFFGetField(in, tag, &v1, &v2, &v3)) TIFFSetField(out, tag, v1, v2, v3)

static	char fname[1024+1];

static	int tiffcp(TIFF*, FILE*); /* geänderte Interfaces! */
static	void newfilename(void);
static	int cpStrips(TIFF*, FILE*);
static	int cpTiles(TIFF*, TIFF*);

static char bFirstLSB=0;

int
main(int argc, char* argv[])
{
	TIFF *in;
	FILE *out;	/* wir müssen das schon selbst machen */

	if (argc < 2) {
		fprintf(stderr, "usage: tiff2bacon input.tif [prefix]\n");
		return (-3);
	}
	if (argc > 2)
		strcpy(fname, argv[2]);
	in = TIFFOpen(argv[1], "r");
	if (in != NULL) {
		do {
			char path[1024+1];
			newfilename();
			strcpy(path, fname);
			if (!argc>2) strcat(path, ".bacon");
			out = /* TIFFOpen */ fopen(path, "wb");
			if (out == NULL)
				return (-2);
			if (!tiffcp(in, out))
				return (-1);
			/* TIFFClose */ close (out);
		} while (TIFFReadDirectory(in));
		(void) TIFFClose(in);
	}
	return (0);
}

static void
newfilename(void)
{
	static int first = 1;
	static long fnum;
	static short defname;
	static char *fpnt;

	if (first) {
		if (fname[0]) {
			fpnt = fname + strlen(fname);
			defname = 0;
		} else {
		        fpnt=fname;
			/* fname[0] = 'x'; */
			/* fpnt = fname + 1; */
			defname = 1;
		}
		first = 0;
	}
#define	MAXFILES	676
	if (fnum == MAXFILES) {
		if (!defname || fname[0] == 'z') {
			fprintf(stderr, "tiffsplit: too many files.\n");
			exit(1);
		}
		fname[0]++;
		fnum = 0;
	}
#ifdef CODE_NAME_ALPHA
	fpnt[0] = fnum / 26 + 'a';
	fpnt[1] = fnum % 26 + 'a';
#else
	sprintf(fpnt,"%03ld",fnum);
#endif
	fnum++;
}

static char achBaconHead[256];

static int
prepBaconHead(uint32 w, uint32 l)
{
	char achNumber[9],i;
	memset(achBaconHead,' ',sizeof(achBaconHead));
	sprintf(achNumber,"%04ld",(long)l);
	for (i=0; i<4; i++) achBaconHead[197+i]=achNumber[i];
	sprintf(achNumber,"%04ld",(long)w);
	for (i=0; i<4; i++) achBaconHead[197+4+i]=achNumber[i];
	
}

static int
tiffcp(TIFF* in, FILE* out) /* auch hier: nicht TIFF */
{
	short bitspersample, samplesperpixel, shortv, *shortav;
	uint32 w, l;
	float floatv;
	char *stringv;
	uint32 longv;

#ifdef COPYFIELDS
	CopyField(TIFFTAG_SUBFILETYPE, longv);
	CopyField(TIFFTAG_TILEWIDTH, w);
	CopyField(TIFFTAG_TILELENGTH, l);
	CopyField(TIFFTAG_IMAGEWIDTH, w);
	CopyField(TIFFTAG_IMAGELENGTH, l);
	CopyField(TIFFTAG_BITSPERSAMPLE, bitspersample);
	CopyField(TIFFTAG_COMPRESSION, shortv);
	CopyField(TIFFTAG_PREDICTOR, shortv);
	CopyField(TIFFTAG_PHOTOMETRIC, shortv);
	CopyField(TIFFTAG_THRESHHOLDING, shortv);
	CopyField(TIFFTAG_FILLORDER, shortv);
	CopyField(TIFFTAG_ORIENTATION, shortv);
	CopyField(TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
	CopyField(TIFFTAG_MINSAMPLEVALUE, shortv);
	CopyField(TIFFTAG_MAXSAMPLEVALUE, shortv);
	CopyField(TIFFTAG_XRESOLUTION, floatv);
	CopyField(TIFFTAG_YRESOLUTION, floatv);
	CopyField(TIFFTAG_GROUP3OPTIONS, longv);
	CopyField(TIFFTAG_GROUP4OPTIONS, longv);
	CopyField(TIFFTAG_RESOLUTIONUNIT, shortv);
	CopyField(TIFFTAG_PLANARCONFIG, shortv);
	CopyField(TIFFTAG_ROWSPERSTRIP, longv);
	CopyField(TIFFTAG_XPOSITION, floatv);
	CopyField(TIFFTAG_YPOSITION, floatv);
	CopyField(TIFFTAG_IMAGEDEPTH, longv);
	CopyField(TIFFTAG_TILEDEPTH, longv);
	CopyField2(TIFFTAG_EXTRASAMPLES, shortv, shortav);
	{ uint16 *red, *green, *blue;
	  CopyField3(TIFFTAG_COLORMAP, red, green, blue);
	}
	{ uint16 shortv2;
	  CopyField2(TIFFTAG_PAGENUMBER, shortv, shortv2);
	}
	CopyField(TIFFTAG_ARTIST, stringv);
	CopyField(TIFFTAG_IMAGEDESCRIPTION, stringv);
	CopyField(TIFFTAG_MAKE, stringv);
	CopyField(TIFFTAG_MODEL, stringv);
	CopyField(TIFFTAG_SOFTWARE, stringv);
	CopyField(TIFFTAG_DATETIME, stringv);
	CopyField(TIFFTAG_HOSTCOMPUTER, stringv);
	CopyField(TIFFTAG_PAGENAME, stringv);
	CopyField(TIFFTAG_DOCUMENTNAME, stringv);
	if (TIFFIsTiled(in))
		return (cpTiles(in, out));
	else
		return (cpStrips(in, out));
#else
	TIFFGetField(in,TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(in,TIFFTAG_IMAGELENGTH, &l);
	TIFFGetField(in,TIFFTAG_BITSPERSAMPLE, &bitspersample);
	TIFFGetField(in,TIFFTAG_FILLORDER, &shortv);
	bFirstLSB=(shortv==2);
	TIFFGetField(in,TIFFTAG_COMPRESSION, &shortv);
	if (shortv!=4) /* G4 */
	  {
	    fprintf(stderr,"fatal: only support for G4 compression!\n");
	    return 1;
	  }
	if (bitspersample!=1)
	  {
	    fprintf(stderr,"fatal: only support for b/w!\n");
	    return 1;
	  }
	if (TIFFIsTiled(in))
	  {
	    fprintf(stderr,"fatal: no support for tiles images!\n");
	    return 1;
	  }
	/* fprintf(stderr,"size is %ld/%ld\n",w,l); */
	prepBaconHead(w,l);
	return cpStrips(in,out);
#endif
}

static int
cpBuffer(unsigned char *pchBuf, long lcchBuf, FILE *out)
{
  sprintf(achBaconHead,"%9ldDEC3",lcchBuf+sizeof(achBaconHead));
  achBaconHead[13]=' '; /* NUL wieder wegpatchen */
  if (sizeof(achBaconHead)!=fwrite(achBaconHead,1,sizeof(achBaconHead),out))
    return 1;
  /*
    fprintf (stderr,"notice: %s\n",bFirstLSB ? "reverse bits" : "normal order");
  */
  while (lcchBuf>0)
    {
      long lcch=lcchBuf;
      if (lcch>0x1000) lcch=0x1000;
      /**
	 Ekelhafterweise werden einige TIFFs mit gespiegelter
	 Bitfolge transportiert, und Ghostscript macht da keine 
	 Ausnahme.
      */
      if (bFirstLSB)
	{
	  int i;
	  for (i=0; i<(int)lcch; i++)
	    {
	      unsigned char chTo;
	      unsigned char chFrom=pchBuf[i];
	      int iBit;
	      for (iBit=0; iBit<8; iBit++)
		{
		  chTo=(chTo<<1)|(chFrom&0x01);
		  chFrom>>=1;
		}
	      pchBuf[i]=chTo;
	    }
	}
      if (lcch!=fwrite(pchBuf,1,(size_t)lcch,out)) return 1;
      pchBuf+=lcch;
      lcchBuf-=lcch;
    }
  return 0;
}

static int
cpStrips(TIFF* in, FILE * out) /* s.o. :-) */
{
	tsize_t bufsize  = TIFFStripSize(in);
	tstrip_t ns = TIFFNumberOfStrips(in);
	unsigned char *buf = (unsigned char *)_TIFFmalloc(bufsize);

	if (buf) {
		uint32 *bytecounts;
		tstrip_t s;

		if (ns!=1)
		  {
		    fprintf(stderr,"fatal: no support for multistrip images\n");
		    return 0;
		  }

		TIFFGetField(in, TIFFTAG_STRIPBYTECOUNTS, &bytecounts);
		for (s = 0; s < ns; s++) {
			if (bytecounts[s] > bufsize) {
				buf = (unsigned char *)_TIFFrealloc(buf, bytecounts[s]);
				if (!buf)
					return (0);
				bufsize = bytecounts[s];
			}
			if (TIFFReadRawStrip(in, s, buf, bytecounts[s]) < 0 ||
		           cpBuffer(buf,bytecounts[s],out))
			  /* TIFFWriteRawStrip(out, s, buf, bytecounts[s]) < 0)*/
			  {
				_TIFFfree(buf);
				return (0);
			  }
		}
		_TIFFfree(buf);
		return (1);
	}
	return (0);
}

static int
cpTiles(TIFF* in, TIFF* out)
{
	tsize_t bufsize = TIFFTileSize(in);
	unsigned char *buf = (unsigned char *)_TIFFmalloc(bufsize);

	if (buf) {
		ttile_t t, nt = TIFFNumberOfTiles(in);
		uint32 *bytecounts;

		TIFFGetField(in, TIFFTAG_TILEBYTECOUNTS, &bytecounts);
		for (t = 0; t < nt; t++) {
			if (bytecounts[t] > bufsize) {
				buf = (unsigned char *)_TIFFrealloc(buf, bytecounts[t]);
				if (!buf)
					return (0);
				bufsize = bytecounts[t];
			}
			if (TIFFReadRawTile(in, t, buf, bytecounts[t]) < 0 ||
			    TIFFWriteRawTile(out, t, buf, bytecounts[t]) < 0) {
				_TIFFfree(buf);
				return (0);
			}
		}
		_TIFFfree(buf);
		return (1);
	}
	return (0);
}
