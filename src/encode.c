/*
*
* $Author: eichholz $
* $Revision: 1.1 $
* $State: Exp $
* $Date: 2001/12/30 10:11:47 $
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
* <encode.c>
*
* Hier wird eine G4-Zeile mit Hilfe des externen Gedächtnisses
* nach aBitWork dekodiert.
*
* EncodePage() macht alles.
*
* UNGETESTET für die Korrekturen >0.13 !!!
*
* ******************************************************* */

#include "global.h"

static unsigned char	uchSymbol;
static int		cbSymbol;

/*
 ******************************************************************
 * WriteSymbol()
 ******************************************************************
 */

static BOOL WriteSymbol(FILE *f, char *szBCB)
 {
 		/**
 		WriteSymbol bekommt eine Bit-Zeichenkette (1 Byte pro Bit)
 		hineingereicht und schreibt sie akkumulierend in uchSymbol
 		auf die Ausgabedatei.
 		*/
  BOOL b;
  if (bDebugEncoder)
    fprintf(stderr," =>(%s)\n",szBCB);
  b=TRUE;
  while (*szBCB && b)
   {
    uchSymbol=(uchSymbol<<1)|((*szBCB)-'0');
    szBCB++;
    cbSymbol++;
    if (cbSymbol==8)
     {
      /* fputc(uchSymbol,f); */
      if (nFileFormat==OFMT_RAWG4)
        putc(uchSymbol,f);
      else
        b=TiffEnter(uchSymbol);
      cbSymbol=0;
     }
   }
  return b;
 }

static BOOL WriteOP(FILE *f, int nID)
 {
  return WriteSymbol(f, arstSpecs[2][nID-1].szBitMask);
 }

/*
 ******************************************************************
 * FlushLineG4()
 ******************************************************************
 */

	/**
	Get Changing Element.
	*/
static BOOL GetCE(BOOL *abit, int x0, int *px1)
 {
  BOOL b0=abit[x0];
  while (x0<=cxPaperOut && abit[x0]==b0)
    x0++;
  *px1=x0;
  return (x0<=cxPaperOut);
 }

static BOOL GetSync(int a0, int *b0)
 {
  int col0=abitWork[a0];
  	/**
  	Achtung, Falle: b1 (b[0]) muß die gleiche Farbe wie a1 haben,
  	also verschieden von a0 UND es reicht nicht, wenn oberhalb von
  	a0 bereits die andere Farbe bereitsteht!
  	*/
  while (a0<=cxPaperOut && abitRef[a0]!=col0)
    a0++;
  *b0=a0;
  return (a0<=cxPaperOut);
 }
 
	/**
	Hier geschieht die zeilenweise Steuerung der Kodierung.
	*/
BOOL FlushLineG4(FILE *f)
 {
  int a[3],b[2];
  BOOL	bScanning;
  a[0]=0;
  abitWork[0]=abitRef[0]=WHITE;
  bScanning=TRUE;
  if (bDebugEncoder)
    fprintf(stderr,"encoding line %d: ",iLine);
  while (bScanning=(a[0]<=cxPaperOut))
   {
    GetCE(abitWork,a[0],a+1);
    GetSync(a[0],b+0);
    GetCE(abitRef,b[0],b+0);
    GetCE(abitRef,b[0],b+1);
    /* if (a[0]>cxPaperOut) bScanning=FALSE; */
    if (bDebugEncoder)
      fprintf(stderr,"got [%d,%d] vs. [%d,%d] ",
      		a[0],a[1],b[0],b[1]);
    if (b[1]<a[1])

     {
      if (bDebugEncoder)
        fprintf(stderr,"->P[%d]",b[1]);
      WriteOP(f,OP_SYM_P);
      a[0]=b[1];
     } /* Pass Mode */

    else

     {
      int nOff=a[1]-b[0];
#define xOTHERWAY_CODED
#ifdef  OTHERWAY_CODED      
      if (!a[0]) nOff--;
#endif      
      if (nOff<=3 && nOff>=-3)
       {
        if (bDebugEncoder)
          fprintf(stderr,"->V(%d)",nOff);
        switch (nOff)
         {
          case -3: WriteOP(f,OP_SYM_VL3); break;
          case -2: WriteOP(f,OP_SYM_VL2); break;
          case -1: WriteOP(f,OP_SYM_VL1); break;
          case  0: WriteOP(f,OP_SYM_V0); break;
          case  1: WriteOP(f,OP_SYM_VR1); break;
          case  2: WriteOP(f,OP_SYM_VR2); break;
          case  3: WriteOP(f,OP_SYM_VR3); break;
         }
        a[0]=a[1];
       } /* vertical */

      else

       {
        int i,bColor;
        WriteOP(f,OP_SYM_H);
        GetCE(abitWork,a[1],a+2);
        bColor=abitWork[a[0]];
        if (bDebugEncoder)
         {
          fprintf(stderr,"->%s(%d,%d)",
          	bColor ? "BW" : "WB",
          	a[1]-a[0],a[2]-a[1]);
         }
        for (i=0; i<2; i++)
         {
          BOOL bForceClose=TRUE; 	/* läßt Null durchgehen */
          nOff=a[i+1]-a[i];
          if (a[i]==0)	/* das ist dann der erste Lauf */
            nOff--;	/* das imaginäre Pel wird NICHT kodiert. */
          	/**
          	Offsetcodes zusammensetzen.
          	Nach einem Makeupcode MUSS (!) ggf. eine Null
          	kodiert werden!!!
          	*/
          while (nOff>0 || bForceClose)
           {
            TTemplRunSpec *t,*tLast;
            tLast=NULL; /* darf kein Fehler werden! */
            t=arstSpecs[bColor]; /* white, black */
            while (t->szBitMask && nOff>=t->cPixel)
             {
              tLast=t++;
             }
            WriteSymbol(f,tLast->szBitMask);
            nOff-=tLast->cPixel;
            bForceClose=(/* !nOff && */tLast->cPixel>63);
           } /* while making up */
          bColor=!bColor;
         } /* for color */
        a[0]=a[2];
       } /* Horizontal */

     }
   } /* pixel avaliable */
  return TRUE;
 } 

/*
 ******************************************************************
 * UnPackLine()
 ******************************************************************
 */

static void UnPackLine()
 {
  int		i,bi;
  unsigned char	*pch;
  pch=pchFullPage+(iLine-1)*lcchFullPageLine+xPaperOut/8;
  abitWork[0]=WHITE;
  for (i=1; i<=cxPaperOut;)
   {
    for (bi=0x80; bi>0; bi=bi>>1)
      abitWork[i++]=!!((*pch) & bi);
    pch++;
   }
 }

/*
 ******************************************************************
 * WriteHeadG4()
 ******************************************************************
 */

BOOL WriteHeadG4(FILE *f, BOOL bTiffHeader)
 {
  if (bTiffHeader) return TiffCreate(200000); /* 4=G4 */
  return TRUE;
 }

/*
 ******************************************************************
 * ClosePageG4()
 ******************************************************************
 */

BOOL ClosePageG4(FILE *f, BOOL bTiff)
 {
  WriteOP(f,OP_SYM_EOFB);
  if (WriteSymbol(f,"0000000")) /* pad and finish (CEIL!!) */
   {
    if (bTiff)
      return TiffClose(f,cxPaperOut,cyPaperOut,4); /* 4=G4 */
    else
      return TRUE;
   }  
  return FALSE;
 }

/*
 ******************************************************************
 * EncodePage()
 ******************************************************************
 */

int EncodePage(void)
 {
  int i;
  if (bVerbose)
    fprintf(stderr,"encoding [%d,%d] ...\n",cxPaperOut,cyPaperOut);
  WriteHeader(stdout);
  abitRef=aabitBuffers[1];
  abitWork=aabitBuffers[0];
  iBuffer=0;
  CenterPage();
  for (iLine=1; iLine<=yPaperOut; iLine++) UnPackLine();
  for (i=0; i<cxPaperOut+3; abitRef[i]=WHITE) i++;
  for (iLine=1; iLine<=cyPaperOut; iLine++)
   {
    UnPackLine();
    FlushLine(stdout);
    iBuffer=(iBuffer+1) & 1;
    abitRef=abitWork;
    abitWork=aabitBuffers[iBuffer];
   }
  ClosePage(stdout);
  return 0;
 }

/* $Extended$File$Info$
 */
