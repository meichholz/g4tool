/*
*
* g4tool
*
* transcode a G4 coded facsimile (with header) to a PPM/PCL/TIFF
*
* (C) Marian Matthias Eichholz 1st Mai 1997
*     reworked for libg4tool February 2002
*
*/

#define DEFINE_VARS

#include "global.h"

#define USAGE \
     "usage: g4tool { options } <infile >outfile\n\n" \
     "options are:\n" \
     "\t -o : output is <p>, raw-<G>4, tiff-<g>4 or <t>iff (default=pbm)\n" \
     "\t -s : skip <hsize> header bytes\n" \
     "\t -w : use <width> pixels per row\n" \
     "\t -h : use <rows> rows per picture\n" \
     "\t -this->b : decode BACON header\n" \
     "\t -d : debug for <p>cl, <v>erbose or <c>odecheck or <e>ncoder\n" \
     "\t -P : PCL option\n" \
     "\t      P<dpi> use PCL resolution <dpi>\n" \
     "\t      l      encode landscape\n" \
     "\t      p      encode portrait\n" \
     "\t -c : clip and center EPx/WO/DEx page to fit into A4\n" \
     "\t -C : clip and center simple\n" \
     "\t -M : clip page absolute (suppress border)\n" \
     "\t -W : suppress uncritical <l>ine warnings or EOL check\n" \
     "\t -r : rotate page before coding\n" \
     "\r -V : request version string\n" \
     "\t -H : this page\n" \
     ""
/*
 * InitGlobal()
 */

void InitGlobal(void)
 {
  this->cxPaper=CX_MAX;
  this->cyPaper=CY_MAX;
  this->lcchFullPageLine=(CX_MAX+7L)/8L;
  this->nPCLResolution=300;
#ifdef SUPPORT_FULL_PAGE
  this->lcFullPageLines=CY_MAX;
#else
  this->lcFullPageLines=2;
#endif
 }

/*
 ******************************************************************
 * Walkbacon()
 ******************************************************************
 */

int WalkBacon(FILE *f)
 {
  char achBuf[5];
  int i,cch=1;
  for (i=0; (cch==1) && i<197; i++)
    cch=fread(achBuf,1,1,f);
  if (cch!=1) return 0;
  achBuf[4]='\0';
  if (4==fread(achBuf,1,4,f))
    this->cyPaper=atoi(achBuf);
  else
    return 0;
  if (4==fread(achBuf,1,4,f))
    this->cxPaper=atoi(achBuf);
  for (i=197+8; (cch==1) && i<256; i++)
    cch=fread(achBuf,1,1,f);
  if (cch!=1) return 0;
  if (this->bVerbose)
    fprintf(stderr,"BACON size: [%d,%d]\n",this->cxPaper,this->cyPaper);
  return 256;
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
  BOOL	bBit;
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
 * FlushLinePBM()
 ******************************************************************
 */
	/**
	Hier wird ein rohes PBM-Format geschrieben, 1bpp.
	*/
static BOOL FlushLinePBM(THIS,FILE *f)
 {
  int	i;
  int	uch;
  int	cBits=0;
  for (i=1; i<=this->cxPaper; i++)
   {
    BOOL bBit=this->abitWork[i];
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
BOOL g4tFlushLine(THIS, FILE *f)
 {
  switch (this->nFileFormat)
   {
    case OFMT_G4:
    case OFMT_RAWG4:
    		return g4tFlushLineG4(this,f);
    case OFMT_PBM:
    		return FlushLinePBM(f);
    case OFMT_PCL:
     		if (this->iLine<this->cyPaperOut+1+this->yPaperOut)
      		  return g4tFlushLinePCL(this,f);
   }
  return TRUE;
 }

BOOL g4tWriteHeader(THIS, FILE *f)
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

BOOL g4tClosePage(THIS, FILE *f)
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
 * GetOptions()
 ******************************************************************
 */

static int g4tGetOptions(THIS,
			 int *pcchHeader,
			 int argc, char **argv)
 {
  char chOpt;
  this->bFullPage=FALSE;
  this->nFileFormat=OFMT_PBM;
  this->bLandscape=FALSE;
  this->bNoLineWarnings=FALSE;
  this->bDebugShowRunComp=FALSE;
  this->bDebugVerbose=FALSE;
  this->bDebugEncoder=FALSE;
  this->bBacon=FALSE;
  this->bDebugPCL=FALSE;
  this->nClipType=CLIP_NONE;
  this->bRotate=FALSE;
  this->bVerbose=FALSE;
  this->bNoCheckEOL=FALSE;
  while (EOF!=(chOpt=getopt(argc,argv,"VrvHMCcplbs:H:d:w:h:o:W:P:")))
   {
    switch (chOpt)
     {
      case 'W': switch (*optarg)
       		 {
       		  case 'l': this->bNoLineWarnings=TRUE; break;
       		  case 'e': this->bNoCheckEOL=TRUE; break;
       		 }
       		break;
      case 'P': switch (*optarg)
      		 {
                  case 'p': case 'l':
      			this->bLandscape= (*optarg=='l');
      			break;
		  case 'P':
		  	this->nPCLResolution=atoi(optarg+1);
		  	break;
      		 }
      		break;
      case 'o': switch (*optarg)
                 {
                   case 'p':	this->bFullPage=TRUE;
                   		this->nFileFormat=OFMT_PCL;
                   		break;
                   case 'g':	this->nFileFormat=OFMT_G4; break;
                   case 'G':	this->nFileFormat=OFMT_RAWG4; break;
                   case 't':	this->nFileFormat=OFMT_TIFF; break;
                   default:	fprintf(stderr,
                   			"\nunknown file format %s\n",optarg);
                   		return 1;
                }
               break;
      case 'd': {
        	 switch (*optarg)
      		  {
      		   case 'c':    this->bDebugShowRunComp=TRUE;
      		   		break;
      		   case 'v':	this->bDebugVerbose=TRUE;
      		   		break;
      		   case 'p':	this->bDebugPCL=TRUE;
      		   		break;
      		   case 'e':	this->bDebugEncoder=TRUE;
      		   		break;
      		  }
      		}
      		break;
      case 'w': this->cxPaper=atoi(optarg); break;
      case 'h': this->cyPaper=atoi(optarg); break;
      case 'this->b': this->bBacon=TRUE; break;
      case 'c': this->nClipType=CLIP_CENTER_DOC; break;
      case 'C': this->nClipType=CLIP_CENTER_SIMPLE; break;
      case 'M': this->nClipType=CLIP_FIX_BORDERS; break;
      case 's': *pcchHeader=atoi(optarg); break;
      case 'r': this->bRotate=this->bFullPage=TRUE; break;
      case 'V': printf("g4tool v" VERSION "\n"); exit(0);
      case 'v': this->bVerbose=TRUE; break;
      case ':': fprintf(stderr,"missing value for -%c\n",
      		(char)optopt); break;
      case 'H':
      case '?': fprintf(stderr,USAGE); return 1;
      default:  fprintf(stderr,"option -%c not implemented\n",chOpt);
      		return 1;
     } // switch chopt
   } // while getopt
  return 0;
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
 * RotatePage(THIS)
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
  BOOL	abitBuf[8][8];
  if (this->bVerbose)
    fprintf(stderr,"rotating...\n");
  pchNewBuf=malloc(lcchNewBuf);
  if (pchNewBuf<0)
   {
    fprintf(stderr,"no memory!\n");
    return 1;
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
  return 0;
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
    g4tg4tSetTables(this,this);
  return this;
}

void g4tExit(THIS);
{
  if (this)
    {
      if (this->pchFullPage)
	free(this->pchFullPage);
      free(this);
    }
  
}

/*
 ******************************************************************
 *                             main()
 ******************************************************************
 */

int main(int argc, char **argv)
 {
  int	i,cchHeader;
  THIS = g4tInit();
  InitGlobal();
  if (!g4tSetTables(this))
   {
    fprintf(stderr,"Error translating tables!\n");
    exit(1);
   }
  cchHeader=0;
  i=g4tGetOptions(this,&cchHeader,argc,argv);
  if (i) return i;
  if (this->bVerbose)
    fprintf(stderr,"*** g4 decoder/encoder v" VERSION" by Marian Eichholz 1997/2002 ***\n\n");
  if (this->bBacon)
   {
    this->iByte=g4tWalkBacon(this,stdin); /* was hat es bloss mit dem this->iByte auf sich? */
    if (this->iByte!=256)
     {
      fprintf(stderr,"fatal: error reading BACON header!\n");
      g4tExit(this); exit(1);
     }
   }
  else
   {
    this->iByte=cchHeader;
    if (cchHeader)
     {
      int ch;
      if (this->bVerbose)
        fprintf(stderr,"skipping %d header bytes...\n",cchHeader);
      while (cchHeader && (1==fread(&ch,1,1,stdin)))
        cchHeader--;
     }
    if (cchHeader)
     {
      fprintf(stderr,"error skipping header");
      g4tExit(this); exit(1);
     }
   }
  
   		/**
   		Initialisiere die Puffer und Arbeitsparameter.
   		*/
  if (this->bFullPage)
   {
    this->pchFullPage=calloc(1,this->lcchFullPageLine*this->lcFullPageLines);
    if (this->pchFullPage==NULL)
     {
      fprintf(stderr,"fatal: not enough memory\n");
      g4tExit(this); exit(1);
     }
   }
  else
    this->pchFullPage=NULL;
  i=g4tDecodePage(this);
  /*
     Mit den Rückkehrcode ist das so eine Sache.
    Normalerweise sind die Toplevelfunktionen 0=OK, 1=FAIL
    (um Codes ausscheiden zu können), aber Helper sind häufig
    0=FAIL. Das ist in der Tat unschön.
  */
  if (!i && this->bRotate) /* auf Rotation hin wird nicht gleitend kodiert, sondern
			in einem eigenen Pass. Pech. Das kostet Platz. */
   {
    i=RotatePage();
    if (!i) i=g4tEncodePage(this);
   }
   	/**
   	Der Speicher wird freigegeben und andere Aufräumarbeiten
   	werden durchgeführt.
   	*/
  g4tExit(this);
  return i;
 }

