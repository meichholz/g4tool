/*
*
* $Author: eichholz $
* $Revision: 1.1 $
* $State: Exp $
* $Date: 2001/12/30 10:11:51 $
*
* ************************************************************
* g4tool
*
* Umwandlung eines G4-Bildes (ggf. mit Header) in ein PPM
*
* (C) Marian Matthias Eichholz 1. Mai 1997
*
*/

/* *******************************************************
*
* <tiff.c>
*
* Mimik zum Handeln eines TIFF-Images
*
* ******************************************************* */

#include "global.h"

static unsigned char *pchWrite;
static long	liWrite;
static long	liMax;

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

BOOL TiffCreate(long lcchEstimate)
 {
  liMax=lcchEstimate;
  pchWrite=malloc(liMax);
  if (pchWrite==0) return FALSE;
  return TRUE;
 }

BOOL TiffEnter(unsigned char uch)
 {
  if (pchWrite && liWrite<liMax)
   {
    pchWrite[liWrite++]=uch;
    return TRUE;
   }
  return FALSE;
 }

static BOOL TiffWriteAny(FILE *f, long l, int cBytes)
 {
  BOOL		b;
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

BOOL TiffWriteShort(FILE *f, short s)
 {
  return TiffWriteAny(f,s,2);
 }

BOOL TiffWriteLong(FILE *f, long l)
 {
  return TiffWriteAny(f,l,4);
 }

BOOL TiffWriteTag(FILE *f, short sTagType, short sDataType,
		long cData, long lData)
 {
  BOOL b=TRUE;
  if (b) b=TiffWriteShort(f,sTagType);
  if (b) b=TiffWriteShort(f,sDataType);
  if (b) b=TiffWriteLong(f,cData);
  if (b) b=TiffWriteLong(f,lData); /* erm: Long/Short+Pad? */
  return b;
 }

BOOL TiffClose(FILE *f, int cx, int cy, int idCompression)
 {
  BOOL	b;
  short	cTags;
  if (!pchWrite) return FALSE;
  fprintf(f,"II");			/* erst mal nur INTEL */
  b=TRUE;
  if (b) b=TiffWriteShort(f,42);
  if (b) b=TiffWriteLong(f,8);
  					/* Nur ein IFD wird angelegt */
  /* Off=8 */
  cTags=10;
  if (b) b=TiffWriteShort(f,cTags);				/* Anzahl Tags */
  /* Off=10 */
  if (b) b=TiffWriteTag(f,0xFE,TIFF_LONG,1,0); 			/* Vollbild */
  if (b) b=TiffWriteTag(f,0x100,TIFF_SHORT,1,cx);		/* Resolution */
  if (b) b=TiffWriteTag(f,0x101,TIFF_SHORT,1,cy);
  if (b) b=TiffWriteTag(f,0x102,TIFF_SHORT,1,1);		/* 1 bps */
  if (b) b=TiffWriteTag(f,0x103,TIFF_SHORT,1,idCompression);	/* G4-Strips */
  if (b) b=TiffWriteTag(f,0x106,TIFF_SHORT,1,0);		/* WB-Kodierung (?) */
  if (b) b=TiffWriteTag(f,0x111,TIFF_LONG,1,14+cTags*12+16);	/* Datenstart */
  if (b) b=TiffWriteTag(f,0x117,TIFF_LONG,1,liWrite);		/* Länge */
  if (b) b=TiffWriteTag(f,0x11A,TIFF_RATIONAL,1,14+cTags*12+0);	/* Resolution */	
  if (b) b=TiffWriteTag(f,0x11B,TIFF_RATIONAL,1,14+cTags*12+8);
  /* Off=10+nTag*12 */
  if (b) b=TiffWriteLong(f,0);	/* IFD Ende */
  	/* DATEN */
  	/* Auflösungen */
  if (b) b=TiffWriteLong(f,300); b=TiffWriteLong(f,1);
  if (b) b=TiffWriteLong(f,300); b=TiffWriteLong(f,1);
  fwrite(pchWrite,liWrite,1,f);
  free(pchWrite);
  return TRUE;
 }
