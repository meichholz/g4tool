/* *******************************************************
*
* <tiff.c>
*
* some barebones mimic for handling TIFF images.
* will possibly go to Sam Leffler's TIFF-Library soem day.
*
* ******************************************************* */

#include "global.h"

#define TIFF_BYTE	1
#define TIFF_ASCII	2
#define TIFF_SHORT	3
#define TIFF_LONG	4
#define TIFF_RATIONAL	5

/*	(rational belegt im Datenbereich 2 Longs: Zähler->Nenner) */

#ifdef STRUCTURES
typedef struct {
	short	sTagType;
	short	sDataType;
	long	lcchData;
	long	liData;
 } TTiffTag;

typedef struct {
  short		scTags;
  TTiffTag	aTags[];
 } TTiffIFD;

#endif

G4T_BOOL g4tTiffCreate(THIS, long lcchEstimate)
 {
  this->liMax=lcchEstimate;
  this->pchWrite=malloc(this->liMax);
  if (this->pchWrite==0) return FALSE;
  return TRUE;
 }

G4T_BOOL g4tTiffEnter(THIS, unsigned char uch)
 {
  if (this->pchWrite && this->liWrite<this->liMax)
   {
    this->pchWrite[this->liWrite++]=uch;
    return TRUE;
   }
  return FALSE;
 }

static G4T_BOOL TiffWriteAny(THIS, FILE *f, long l, int cBytes)
 {
  G4T_BOOL		b;
  unsigned char	*ptr;
  long lBuffer	=l;
  ptr=(unsigned char *)&lBuffer;
  b=TRUE;
  /* ggf. muß hier permutiert werden!!! */
  		/* nur intel */
  /* Die Bytes des Puffers werden nun einfach weggeschrieben */
  b=(1==fwrite(ptr,cBytes,1,f));
  return b;
 }

G4T_BOOL TiffWriteShort(THIS, FILE *f, short s)
 {
  return TiffWriteAny(this,f,s,2);
 }

G4T_BOOL TiffWriteLong(THIS, FILE *f, long l)
 {
  return TiffWriteAny(this,f,l,4);
 }

G4T_BOOL TiffWriteTag(THIS, FILE *f, short sTagType, short sDataType,
		long cData, long lData)
 {
  G4T_BOOL b=TRUE;
  if (b) b=TiffWriteShort(this,f,sTagType);
  if (b) b=TiffWriteShort(this,f,sDataType);
  if (b) b=TiffWriteLong(this,f,cData);
  if (b) b=TiffWriteLong(this,f,lData); /* erm: Long/Short+Pad? */
  return b;
 }

G4T_BOOL g4tTiffClose(THIS, FILE *f, int cx, int cy, int idCompression)
 {
  G4T_BOOL	b;
  short	cTags;
  if (!this->pchWrite) return FALSE;
  fprintf(f,"II");			/* erst mal nur INTEL */
  b=TRUE;
  if (b) b=TiffWriteShort(this,f,42);
  if (b) b=TiffWriteLong(this,f,8);
  					/* Nur ein IFD wird angelegt */
  /* Off=8 */
  cTags=10;
  if (b) b=TiffWriteShort(this,f,cTags);				/* Anzahl Tags */
  /* Off=10 */
  if (b) b=TiffWriteTag(this,f,0xFE,TIFF_LONG,1,0); 			/* Vollbild */
  if (b) b=TiffWriteTag(this,f,0x100,TIFF_SHORT,1,cx);		/* Resolution */
  if (b) b=TiffWriteTag(this,f,0x101,TIFF_SHORT,1,cy);
  if (b) b=TiffWriteTag(this,f,0x102,TIFF_SHORT,1,1);		/* 1 bps */
  if (b) b=TiffWriteTag(this,f,0x103,TIFF_SHORT,1,idCompression);	/* G4-Strips */
  if (b) b=TiffWriteTag(this,f,0x106,TIFF_SHORT,1,0);		/* WB-Kodierung (?) */
  if (b) b=TiffWriteTag(this,f,0x111,TIFF_LONG,1,14+cTags*12+16);	/* Datenstart */
  if (b) b=TiffWriteTag(this,f,0x117,TIFF_LONG,1,this->liWrite);		/* Länge */
  if (b) b=TiffWriteTag(this,f,0x11A,TIFF_RATIONAL,1,14+cTags*12+0);	/* Resolution */	
  if (b) b=TiffWriteTag(this,f,0x11B,TIFF_RATIONAL,1,14+cTags*12+8);
  /* Off=10+nTag*12 */
  if (b) b=TiffWriteLong(this,f,0);	/* IFD Ende */
  	/* DATEN */
  	/* Auflösungen */
  if (b) b=TiffWriteLong(this,f,300); b=TiffWriteLong(this,f,1);
  if (b) b=TiffWriteLong(this,f,300); b=TiffWriteLong(this,f,1);
  fwrite(this->pchWrite,this->liWrite,1,f);
  free(this->pchWrite);
  return TRUE;
 }
