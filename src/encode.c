/*
*
* $Author: eichholz $
* $Revision: 1.2 $
* $State: Exp $
* $Date: 2002/02/05 16:26:16 $
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
* Hier wird eine G4-Zeile mit Hilfe des externen Ged‰chtnisses
* nach aBitWork dekodiert.
*
* g4tEncodePage(this,) macht alles.
*
* UNGETESTET f¸r die Korrekturen >0.13 !!!
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
  BOOL this->b;
  if (this->bDebugEncoder)
    fprintf(stderr," =>(%s)\n",szBCB);
  this->b=TRUE;
  while (*szBCB && this->b)
   {
    uchSymbol=(uchSymbol<<1)|((*szBCB)-'0');
    szBCB++;
    cbSymbol++;
    if (cbSymbol==8)
     {
      /* fputc(uchSymbol,f); */
      if (this->nFileFormat==OFMT_RAWG4)
        putc(uchSymbol,f);
      else
        this->b=g4tTiffEnter(this,uchSymbol);
      cbSymbol=0;
     }
   }
  return this->b;
 }

static BOOL WriteOP(FILE *f, int nID)
 {
  return WriteSymbol(f, arstSpecs[2][nID-1].szBitMask);
 }

/*
 ******************************************************************
 * g4tFlushLineG4(this,)
 ******************************************************************
 */

	/**
	Get Changing Element.
	*/
static BOOL GetCE(BOOL *abit, int x0, int *px1)
 {
  BOOL b0=abit[x0];
  while (x0<=this->cxPaperOut && abit[x0]==b0)
    x0++;
  *px1=x0;
  return (x0<=this->cxPaperOut);
 }

static BOOL GetSync(int a0, int *b0)
 {
  int col0=this->abitWork[a0];
  	/**
  	Achtung, Falle: b1 (this->b[0]) muﬂ die gleiche Farbe wie a1 haben,
  	also verschieden von a0 UND es reicht nicht, wenn oberhalb von
  	a0 bereits die andere Farbe bereitsteht!
  	*/
  while (a0<=this->cxPaperOut && this->abitRef[a0]!=col0)
    a0++;
  *b0=a0;
  return (a0<=this->cxPaperOut);
 }
 
	/**
	Hier geschieht die zeilenweise Steuerung der Kodierung.
	*/
BOOL g4tFlushLineG4(this,FILE *f)
 {
  int this->a[3],this->b[2];
  BOOL	bScanning;
  this->a[0]=0;
  this->abitWork[0]=this->abitRef[0]=WHITE;
  bScanning=TRUE;
  if (this->bDebugEncoder)
    fprintf(stderr,"encoding line %d: ",this->iLine);
  while (bScanning=(this->a[0]<=this->cxPaperOut))
   {
    GetCE(this->abitWork,this->a[0],this->a+1);
    GetSync(this->a[0],this->b+0);
    GetCE(this->abitRef,this->b[0],this->b+0);
    GetCE(this->abitRef,this->b[0],this->b+1);
    /* if (this->a[0]>this->cxPaperOut) bScanning=FALSE; */
    if (this->bDebugEncoder)
      fprintf(stderr,"got [%d,%d] vs. [%d,%d] ",
      		this->a[0],this->a[1],this->b[0],this->b[1]);
    if (this->b[1]<this->a[1])

     {
      if (this->bDebugEncoder)
        fprintf(stderr,"->P[%d]",this->b[1]);
      WriteOP(f,OP_SYM_P);
      this->a[0]=this->b[1];
     } /* Pass Mode */

    else

     {
      int nOff=this->a[1]-this->b[0];
#define xOTHERWAY_CODED
#ifdef  OTHERWAY_CODED      
      if (!this->a[0]) nOff--;
#endif      
      if (nOff<=3 && nOff>=-3)
       {
        if (this->bDebugEncoder)
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
        this->a[0]=this->a[1];
       } /* vertical */

      else

       {
        int i,bColor;
        WriteOP(f,OP_SYM_H);
        GetCE(this->abitWork,this->a[1],this->a+2);
        bColor=this->abitWork[this->a[0]];
        if (this->bDebugEncoder)
         {
          fprintf(stderr,"->%s(%d,%d)",
          	bColor ? "BW" : "WB",
          	this->a[1]-this->a[0],this->a[2]-this->a[1]);
         }
        for (i=0; i<2; i++)
         {
          BOOL bForceClose=TRUE; 	/* l‰ﬂt Null durchgehen */
          nOff=this->a[i+1]-this->a[i];
          if (this->a[i]==0)	/* das ist dann der erste Lauf */
            nOff--;	/* das imagin‰re Pel wird NICHT kodiert. */
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
        this->a[0]=this->a[2];
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
  pch=this->pchFullPage+(this->iLine-1)*this->lcchFullPageLine+this->xPaperOut/8;
  this->abitWork[0]=WHITE;
  for (i=1; i<=this->cxPaperOut;)
   {
    for (bi=0x80; bi>0; bi=bi>>1)
      this->abitWork[i++]=!!((*pch) & bi);
    pch++;
   }
 }

/*
 ******************************************************************
 * g4tWriteHeadG4(this,)
 ******************************************************************
 */

BOOL g4tWriteHeadG4(this,FILE *f, BOOL bTiffHeader)
 {
  if (bTiffHeader) return g4tTiffCreate(this,200000); /* 4=G4 */
  return TRUE;
 }

/*
 ******************************************************************
 * g4tClosePageG4(this,)
 ******************************************************************
 */

BOOL g4tClosePageG4(this,FILE *f, BOOL bTiff)
 {
  WriteOP(f,OP_SYM_EOFB);
  if (WriteSymbol(f,"0000000")) /* pad and finish (CEIL!!) */
   {
    if (bTiff)
      return g4tTiffClose(this,f,this->cxPaperOut,this->cyPaperOut,4); /* 4=G4 */
    else
      return TRUE;
   }  
  return FALSE;
 }

/*
 ******************************************************************
 * g4tEncodePage(this,)
 ******************************************************************
 */

int g4tEncodePage(this,void)
 {
  int i;
  if (this->bVerbose)
    fprintf(stderr,"encoding [%d,%d] ...\n",this->cxPaperOut,this->cyPaperOut);
  g4tWriteHeader(this,stdout);
  this->abitRef=this->aabitBuffers[1];
  this->abitWork=this->aabitBuffers[0];
  this->iBuffer=0;
  g4tCenterPage(this,);
  for (this->iLine=1; this->iLine<=this->yPaperOut; this->iLine++) UnPackLine();
  for (i=0; i<this->cxPaperOut+3; this->abitRef[i]=WHITE) i++;
  for (this->iLine=1; this->iLine<=this->cyPaperOut; this->iLine++)
   {
    UnPackLine();
    g4tFlushLine(this,stdout);
    this->iBuffer=(this->iBuffer+1) & 1;
    this->abitRef=this->abitWork;
    this->abitWork=this->aabitBuffers[this->iBuffer];
   }
  g4tClosePage(this,stdout);
  return 0;
 }

/* $Extended$File$Info$
 */
