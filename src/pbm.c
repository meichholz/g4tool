/* *******************************************************
*
* <pbm.c>
*
* output for PBM format
*
* ******************************************************* */

#include "global.h"

/*
 ******************************************************************
 * g4tFlushLinePBM()
 ******************************************************************
 */
	/**
	Hier wird ein rohes PBM-Format geschrieben, 1bpp.
	*/
G4T_BOOL g4tFlushLinePBM(THIS,FILE *f)
 {
  int	i;
  int	uch;
  int	cBits=0;
  for (i=1; i<=this->cxPaper; i++)
   {
    G4T_BOOL bBit=this->abitWork[i];
    uch=(uch<<1) | bBit;
    	/**
    	Vielleicht ist hier noch Luft, wenn
    	die Einzelwrites von der Applikation gepuffert werden.
    	*/
    if (++cBits == 8)
     {
      fputc((int)uch,f);
      cBits=0;
     }
   }
  /** padden, und Zeile abschließen */
  if (cBits)
   {
    uch=(uch<<(8-cBits));
    fputc((int)uch,f);
   }
  return TRUE;
 }

/*
 ******************************************************************
 * g4tFlushLine(THIS)
 ******************************************************************
 */
 	/**
 	Treiber für alle direkten Formate.
 	*/
G4T_BOOL g4tFlushLine(THIS, FILE *f)
 {
  switch (this->nFileFormat)
   {
    case OFMT_G4:
    case OFMT_RAWG4:
    		return g4tFlushLineG4(this,f);
    case OFMT_PBM:
    		return g4tFlushLinePBM(this,f);
    case OFMT_PCL:
     		if (this->iLine<this->cyPaperOut+1+this->yPaperOut)
      		  return g4tFlushLinePCL(this,f);
   }
  return TRUE;
 }

G4T_BOOL g4tWriteHeader(THIS, FILE *f)
 { 
  switch (this->nFileFormat)
   {
    case OFMT_G4:	return g4tWriteHeadG4(this,f,TRUE);
    case OFMT_RAWG4:	return g4tWriteHeadG4(this,f,FALSE);
    case OFMT_PBM:	fprintf(f,"P4\n%d %d\n",this->cxPaper,this->cyPaper);
    			return TRUE;
    case OFMT_PCL:	return g4tWriteHeadPCL(this,f);
   }
  return FALSE;
 }

G4T_BOOL g4tClosePage(THIS, FILE *f)
 {
  switch (this->nFileFormat)
   {
    case OFMT_G4:	return g4tClosePageG4(this,f,TRUE);
    case OFMT_RAWG4:	return g4tClosePageG4(this,f,FALSE);
    case OFMT_PBM:	return TRUE;
    case OFMT_PCL:	return g4tDumpPagePCL(this,f);
   }
  return FALSE;
 }

/*
 ******************************************************************
 * g4tCenterPage(THIS)
 ******************************************************************
 */

void g4tCenterPage(THIS)
 {
  this->cxPaperOut=this->cxPaper;
  this->xPaperOut=0;
  this->cyPaperOut=this->cyPaper;
  this->yPaperOut=0;
  switch (this->nClipType)
   {
    case CLIP_NONE: break;
    case CLIP_CENTER_DOC:
    case CLIP_CENTER_SIMPLE:
        /**
        Notfalls muß die Seite zentriert werden. Das geschieht mit etwas
        Augenmaß, da die Dokumente ihre Tücken haben, bes. der Barcode
        an DE-Schriften.
        */
        if (this->cxPaper>CX_MAX_A4)
         {
          this->cxPaperOut=CX_MAX_A4;
#ifdef OLD_DOC_CLIP_FORMULA
          this->xPaperOut=(this->cxPaper-this->cxPaperOut)*100
	    /(200-66*(this->nClipType==CLIP_CENTER_SIMPLE));
/*            ^^^^
              these are 134 for "simple"
*/
#else
          this->xPaperOut=(this->cxPaper-this->cxPaperOut)*100/(150-16*(this->nClipType==CLIP_CENTER_SIMPLE));
#endif
          this->xPaperOut-=(this->xPaperOut % 8);
          if (this->bVerbose)
            fprintf(stderr,"limiting X to %d / %d pixels\n",
            	this->xPaperOut,this->cxPaperOut);
         }
        if (this->cyPaper>CY_MAX_A4)
         {
          this->cyPaperOut=CY_MAX_A4;
          this->yPaperOut=(this->cyPaper-this->cyPaperOut)/3;
          /* this->yPaperOut-=(this->yPaperOut % 8); */
          if (this->bVerbose)
            fprintf(stderr,"limiting Y to %d / %d pixels\n",
            	this->yPaperOut,this->cyPaperOut);
         }
       break;
     case CLIP_FIX_BORDERS:
	/* fprintf(stderr,"Wir hatten: %d/%d",this->xPaperOut,this->yPaperOut); */
     	this->xPaperOut=this->nPCLResolution/5;
        this->xPaperOut-=(this->xPaperOut % 8);
     	this->yPaperOut=this->nPCLResolution/7; /* xxx */
	/* fprintf(stderr," und bekommen: %d/%d\n",this->xPaperOut,this->yPaperOut); */
     	break;
   }
 }
   
/*
 ******************************************************************
 * ERRORCODE = RotatePage(THIS)
 ******************************************************************
 */

static void Swap(int *x, int *y)
 {
  int n;
  n = *x; *x = *y; *y = n;
 }

int g4tRotatePage(THIS)
 {
 	/**
 	Es muß erst ein neuer Puffer geholt werden,
 	dann wird er bitweise verfüllt, und am Ende
 	wird er aktueller Puffer.
 	*/
  long 			lcchNewLine=(this->cyPaper+7L)/8L;
  long 			lcchNewBuf=this->cxPaper*lcchNewLine;
  unsigned char		*pchNewBuf;
  int			x0,y0,x,y;
  G4T_BOOL	        abitBuf[8][8];
  if (this->bVerbose)
    fprintf(stderr,"rotating...\n");
  pchNewBuf=malloc(lcchNewBuf);
  if (pchNewBuf<0)
   {
    fprintf(stderr,"no memory!\n");
    return G4T_EGENERAL;
   }
  	/**
  	Der Quellpuffer wird zu Schachbrettchen abgearbeitet.
  	*/
  for (x0=0; x0<this->cxPaper; x0+=8)
    for (y0=0; y0<this->cyPaper; y0+=8)
     {
     	/**
     	Zuerst wird ein Quellkästchen geholt, mit Überlappungsschutz unten.
     	*/
       {
        unsigned char uch;
        int yMax=this->cyPaper-y0;
        if (yMax>8) yMax=8;
        for (y=0; y<8; y++)
         {
          if (y>yMax)
            uch=0;
          else
            uch=this->pchFullPage[(y0+y)*this->lcchFullPageLine+x0/8];
          for (x=0; x<8; x++)
            abitBuf[x][y]=(uch & (0x80>>x))!=0;
         }
       } /* read */
       	/**
       	Dann wird das Kästchen geschrieben.
       	*/
       {
        unsigned char uch;
        int y1;
        int x1;
        x1=this->cyPaper-y0;
        y1=x0;
        for (y=0; y<8; y++) /* y ist nun vertikal! */
         {
          uch=0;
          for (x=0; x<8; x++)
            uch=(uch<<1)|abitBuf[y][7-x];
          if (y1<this->cxPaper)
            pchNewBuf[lcchNewLine*(y1+y)+(x1/8L)]=uch;
         }
       } /* write */
     } /* y0 */
   /* x0 */
#ifdef DIRECT_DUMP
  printf("P4\n%d %d\n",this->cyPaper,this->cxPaper);
  fwrite(pchNewBuf,lcchNewBuf,1,stdout);
  free(pchNewBuf);
#endif
  free(this->pchFullPage);
  this->pchFullPage=pchNewBuf;
  Swap(&this->cxPaper,&this->cyPaper);
  Swap(&this->xPaperOut,&this->yPaperOut);
  Swap(&this->cxPaperOut,&this->cyPaperOut);
  this->lcchFullPageLine=lcchNewLine;
  this->lcFullPageLines=this->cyPaper;
  return G4T_EOK;
 }

/*
 *                     A U S G A B E T E I L
 */

/*
 ******************************************************************
 * g4tEncodePageLine(THIS)
 ******************************************************************
 */

	/**
	Es wird nur die aktuelle Zeile zu Bytes zusammengepackt
	und in den Seitenpuffer übertragen.
	Funktioniert nur mit SUPPORT_FULL_PAGE
	*/
void g4tEncodePageLine(THIS)
 {
#ifdef SUPPORT_FULL_PAGE
  unsigned char	*pchCurrent, uch;
  int	i;
  G4T_BOOL	bBit;
  int	cBits; cBits=0;
  pchCurrent=this->pchFullPage+this->lcchFullPageLine*(this->iLine-1);
  for (i=1; i<=this->cxPaper; i++) /* erste und letzte Bits sind ungültig! */

   {
    bBit=this->abitWork[i];
    uch=(uch<<1) | bBit;
    if (++cBits == 8)
     {
      cBits=0;
      *pchCurrent++ = uch;
     }
   }
  /** padden, und Zeile abschließen */
  if (cBits)
   {
    uch=(uch<<(8-cBits));
    *pchCurrent=uch;
   }
#endif
 }

/*
 ******************************************************************
 *                             g4tInit() and g4tExit()
 ******************************************************************
 */

struct Tg4tInstance *g4tInit(void)
{
  struct Tg4tInstance *this=calloc(1,sizeof(struct Tg4tInstance));
  if (this)
    g4tSetTables(this);
  return this;
}

void g4tExit(THIS)
{
  if (this)
    {
      if (this->pchFullPage)
	free(this->pchFullPage);
      free(this);
    }
}

