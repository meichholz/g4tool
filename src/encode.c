/* *******************************************************
*
* <encode.c>
*
* A G4 row is decoded ti <aBitWork> using some external memory...
* real work is done by g4tEncodePage(this).
*
* untested for changes after 0.13 ?!?
*
* ******************************************************* */

#include "global.h"

/*
 ******************************************************************
 * WriteSymbol()
 ******************************************************************
 */

static G4T_BOOL WriteSymbol(THIS, FILE *f, char *szBCB)
 {
 		/**
 		WriteSymbol bekommt eine Bit-Zeichenkette (1 Byte pro Bit)
 		hineingereicht und schreibt sie akkumulierend in this->uchSymbol
 		auf die Ausgabedatei.
 		*/
  G4T_BOOL b;
  if (this->bDebugEncoder)
    fprintf(stderr," =>(%s)\n",szBCB);
  b=TRUE;
  while (*szBCB && b)
   {
    this->uchSymbol=(this->uchSymbol<<1)|((*szBCB)-'0');
    szBCB++;
    this->cbSymbol++;
    if (this->cbSymbol==8)
     {
      /* fputc(this->uchSymbol,f); */
      if (this->nFileFormat==OFMT_RAWG4)
        putc(this->uchSymbol,f);
      else
        this->b=g4tTiffEnter(this,this->uchSymbol);
      this->cbSymbol=0;
     }
   }
  return b;
 }

static G4T_BOOL WriteOP(THIS,FILE *f, int nID)
 {
  return WriteSymbol(this, f, arstSpecs[2][nID-1].szBitMask);
 }

/*
 ******************************************************************
 * g4tFlushLineG4(this)
 ******************************************************************
 */

	/**
	Get Changing Element.
	*/
static G4T_BOOL GetCE(THIS, G4T_BOOL *abit, int x0, int *px1)
 {
  G4T_BOOL b0=abit[x0];
  while (x0<=this->cxPaperOut && abit[x0]==b0)
    x0++;
  *px1=x0;
  return (x0<=this->cxPaperOut);
 }

static G4T_BOOL GetSync(THIS, int a0, int *b0)
 {
  int col0=this->abitWork[a0];
  	/**
  	Achtung, Falle: b1 (b[0]) muﬂ die gleiche Farbe wie a1 haben,
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
G4T_BOOL g4tFlushLineG4(THIS, FILE *f)
 {
  int a[3],b[2];
  G4T_BOOL	bScanning;
  a[0]=0;
  this->abitWork[0]=this->abitRef[0]=WHITE;
  bScanning=TRUE;
  if (this->bDebugEncoder)
    fprintf(stderr,"encoding line %d: ",this->iLine);
  while (bScanning=(a[0]<=this->cxPaperOut))
   {
    GetCE(this, this->abitWork,a[0],a+1);
    GetSync(this, a[0],b+0);
    GetCE(this,this->abitRef,b[0],b+0);
    GetCE(this,this->abitRef,b[0],b+1);
    /* if (a[0]>this->cxPaperOut) bScanning=FALSE; */
    if (this->bDebugEncoder)
      fprintf(stderr,"got [%d,%d] vs. [%d,%d] ",
      		a[0],a[1],b[0],b[1]);
    if (b[1]<a[1])

     {
      if (this->bDebugEncoder)
        fprintf(stderr,"->P[%d]",b[1]);
      WriteOP(this,f,OP_SYM_P);
      a[0]=b[1];
     } /* Pass Mode */

    else

     {
      int nOff=a[1]-b[0];
      if (nOff<=3 && nOff>=-3)
       {
        if (this->bDebugEncoder)
          fprintf(stderr,"->V(%d)",nOff);
        switch (nOff)
         {
          case -3: WriteOP(this,f,OP_SYM_VL3); break;
          case -2: WriteOP(this,f,OP_SYM_VL2); break;
          case -1: WriteOP(this,f,OP_SYM_VL1); break;
          case  0: WriteOP(this,f,OP_SYM_V0); break;
          case  1: WriteOP(this,f,OP_SYM_VR1); break;
          case  2: WriteOP(this,f,OP_SYM_VR2); break;
          case  3: WriteOP(this,f,OP_SYM_VR3); break;
         }
        a[0]=a[1];
       } /* vertical */

      else

       {
        int i,bColor;
        WriteOP(this,f,OP_SYM_H);
        GetCE(this, this->abitWork,a[1],a+2);
        bColor=this->abitWork[a[0]];
        if (this->bDebugEncoder)
         {
          fprintf(stderr,"->%s(%d,%d)",
          	bColor ? "BW" : "WB",
          	a[1]-a[0],a[2]-a[1]);
         }
        for (i=0; i<2; i++)
         {
          G4T_BOOL bForceClose=TRUE; 	/* l‰ﬂt Null durchgehen */
          nOff=a[i+1]-a[i];
          if (a[i]==0)	/* das ist dann der erste Lauf */
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
            WriteSymbol(this,f,tLast->szBitMask);
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

static void UnPackLine(THIS)
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
 * g4tWriteHeadG4(...)
 ******************************************************************
 */

G4T_BOOL g4tWriteHeadG4(THIS,FILE *f, G4T_BOOL bTiffHeader)
 {
  if (bTiffHeader) return g4tTiffCreate(this,200000); /* 4=G4 */
  return TRUE;
 }

/*
 ******************************************************************
 * g4tClosePageG4(this,)
 ******************************************************************
 */

G4T_BOOL g4tClosePageG4(THIS, FILE *f, G4T_BOOL bTiff)
 {
  WriteOP(this,f,OP_SYM_EOFB);
  if (WriteSymbol(this,f,"0000000")) /* pad and finish (CEIL!!) */
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

G4T_RC g4tEncodePage(THIS, FILE *f)
 {
  int i;
  if (this->bVerbose)
    fprintf(stderr,"encoding [%d,%d] ...\n",this->cxPaperOut,this->cyPaperOut);
  g4tWriteHeader(this,f);
  this->abitRef=this->aabitBuffers[1];
  this->abitWork=this->aabitBuffers[0];
  this->iBuffer=0;
  g4tCenterPage(this);
  for (this->iLine=1; this->iLine<=this->yPaperOut; this->iLine++)
    UnPackLine(this);
  for (i=0; i<this->cxPaperOut+3; this->abitRef[i]=WHITE) i++;
  for (this->iLine=1; this->iLine<=this->cyPaperOut; this->iLine++)
   {
    UnPackLine(this);
    g4tFlushLine(this,f);
    this->iBuffer=(this->iBuffer+1) & 1;
    this->abitRef=this->abitWork;
    this->abitWork=this->aabitBuffers[this->iBuffer];
   }
  g4tClosePage(this,f);
  return G4T_EOK;
 }

/* $Extended$File$Info$
 */
