/*
*
* $Author: eichholz $
* $Revision: 1.1 $
* $State: Exp $
* $Date: 2001/12/30 10:11:50 $
*
* ************************************************************
* g4tool
*
* Umwandlung eines G4-Bildes (ggf. mit Header) in ein PPM
*
* (C) Marian Matthias Eichholz 1. Mai 1997
*
*/

#define DEFINE_VARS

#include "global.h"

static int	cchHeader;

#define USAGE \
     "usage: g4tool { options } <infile >outfile\n\n" \
     "options are:\n" \
     "\t -o : output is <p>, raw-<G>4, tiff-<g>4 or <t>iff (default=pbm)\n" \
     "\t -s : skip <hsize> header bytes\n" \
     "\t -w : use <width> pixels per row\n" \
     "\t -h : use <rows> rows per picture\n" \
     "\t -b : decode BACON header\n" \
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
  cxPaper=CX_MAX;
  cyPaper=CY_MAX;
  lcchFullPageLine=(CX_MAX+7L)/8L;
  nPCLResolution=300;
#ifdef SUPPORT_FULL_PAGE
  lcFullPageLines=CY_MAX;
#else
  lcFullPageLines=2;
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
    cyPaper=atoi(achBuf);
  else
    return 0;
  if (4==fread(achBuf,1,4,f))
    cxPaper=atoi(achBuf);
  for (i=197+8; (cch==1) && i<256; i++)
    cch=fread(achBuf,1,1,f);
  if (cch!=1) return 0;
  if (bVerbose)
    fprintf(stderr,"BACON size: [%d,%d]\n",cxPaper,cyPaper);
  return 256;
 }

/*
 *                     A U S G A B E T E I L
 */

/*
 ******************************************************************
 * EncodePageLine()
 ******************************************************************
 */

	/**
	Es wird nur die aktuelle Zeile zu Bytes zusammengepackt
	und in den Seitenpuffer übertragen.
	Funktioniert nur mit SUPPORT_FULL_PAGE
	*/
void EncodePageLine()
 {
#ifdef SUPPORT_FULL_PAGE
  unsigned char	*pchCurrent, uch;
  int	i;
  BOOL	bBit;
  int	cBits; cBits=0;
  pchCurrent=pchFullPage+lcchFullPageLine*(iLine-1);
  for (i=1; i<=cxPaper; i++) /* erste und letzte Bits sind ungültig! */

   {
    bBit=abitWork[i];
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
static BOOL FlushLinePBM(FILE *f)
 {
  int	i;
  int	uch;
  int	cBits=0;
  for (i=1; i<=cxPaper; i++)
   {
    BOOL bBit=abitWork[i];
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
 * FlushLine()
 ******************************************************************
 */
 	/**
 	Treiber für alle direkten Formate.
 	*/
BOOL FlushLine(FILE *f)
 {
  switch (nFileFormat)
   {
    case OFMT_G4:
    case OFMT_RAWG4:
    		return FlushLineG4(f);
    case OFMT_PBM:
    		return FlushLinePBM(f);
    case OFMT_PCL:
     		if (iLine<cyPaperOut+1+yPaperOut)
      		  return FlushLinePCL(f);
   }
  return TRUE;
 }

BOOL WriteHeader(FILE *f)
 { 
  switch (nFileFormat)
   {
    case OFMT_G4:	return WriteHeadG4(f,TRUE);
    case OFMT_RAWG4:	return WriteHeadG4(f,FALSE);
    case OFMT_PBM:	fprintf(f,"P4\n%d %d\n",cxPaper,cyPaper);
    			return TRUE;
    case OFMT_PCL:	return WriteHeadPCL(f);
   }
  return FALSE;
 }

BOOL ClosePage(FILE *f)
 {
  switch (nFileFormat)
   {
    case OFMT_G4:	return ClosePageG4(f,TRUE);
    case OFMT_RAWG4:	return ClosePageG4(f,FALSE);
    case OFMT_PBM:	return TRUE;
    case OFMT_PCL:	return DumpPagePCL(f);
   }
  return FALSE;
 }

/*
 ******************************************************************
 * GetOptions()
 ******************************************************************
 */

int GetOptions(int argc, char **argv)
 {
  char chOpt;
  bFullPage=FALSE;
  nFileFormat=OFMT_PBM;
  bLandscape=FALSE;
  bNoLineWarnings=FALSE;
  bDebugShowRunComp=FALSE;
  bDebugVerbose=FALSE;
  bDebugEncoder=FALSE;
  bBacon=FALSE;
  bDebugPCL=FALSE;
  nClipType=CLIP_NONE;
  bRotate=FALSE;
  bVerbose=FALSE;
  bNoCheckEOL=FALSE;
  cchHeader=0;
  while (EOF!=(chOpt=getopt(argc,argv,"VrvHMCcplbs:H:d:w:h:o:W:P:")))
   {
    switch (chOpt)
     {
      case 'W': switch (*optarg)
       		 {
       		  case 'l': bNoLineWarnings=TRUE; break;
       		  case 'e': bNoCheckEOL=TRUE; break;
       		 }
       		break;
      case 'P': switch (*optarg)
      		 {
                  case 'p': case 'l':
      			bLandscape= (*optarg=='l');
      			break;
		  case 'P':
		  	nPCLResolution=atoi(optarg+1);
		  	break;
      		 }
      		break;
      case 'o': switch (*optarg)
                 {
                   case 'p':	bFullPage=TRUE;
                   		nFileFormat=OFMT_PCL;
                   		break;
                   case 'g':	nFileFormat=OFMT_G4; break;
                   case 'G':	nFileFormat=OFMT_RAWG4; break;
                   case 't':	nFileFormat=OFMT_TIFF; break;
                   default:	fprintf(stderr,
                   			"\nunknown file format %s\n",optarg);
                   		return 1;
                }
               break;
      case 'd': {
        	 switch (*optarg)
      		  {
      		   case 'c':    bDebugShowRunComp=TRUE;
      		   		break;
      		   case 'v':	bDebugVerbose=TRUE;
      		   		break;
      		   case 'p':	bDebugPCL=TRUE;
      		   		break;
      		   case 'e':	bDebugEncoder=TRUE;
      		   		break;
      		  }
      		}
      		break;
      case 'w': cxPaper=atoi(optarg); break;
      case 'h': cyPaper=atoi(optarg); break;
      case 'b': bBacon=TRUE; break;
      case 'c': nClipType=CLIP_CENTER_DOC; break;
      case 'C': nClipType=CLIP_CENTER_SIMPLE; break;
      case 'M': nClipType=CLIP_FIX_BORDERS; break;
      case 's': cchHeader=atoi(optarg); break;
      case 'r': bRotate=bFullPage=TRUE; break;
      case 'V': printf("g4tool v" VERSION "\n"); exit(0);
      case 'v': bVerbose=TRUE; break;
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
 * CenterPage()
 ******************************************************************
 */

void CenterPage(void)
 {
  cxPaperOut=cxPaper;
  xPaperOut=0;
  cyPaperOut=cyPaper;
  yPaperOut=0;
  switch (nClipType)
   {
    case CLIP_NONE: break;
    case CLIP_CENTER_DOC:
    case CLIP_CENTER_SIMPLE:
        /**
        Notfalls muß die Seite zentriert werden. Das geschieht mit etwas
        Augenmaß, da die Dokumente ihre Tücken haben, bes. der Barcode
        an DE-Schriften.
        */
        if (cxPaper>CX_MAX_A4)
         {
          cxPaperOut=CX_MAX_A4;
#ifdef OLD_DOC_CLIP_FORMULA
          xPaperOut=(cxPaper-cxPaperOut)*100/(200-66*(nClipType==CLIP_CENTER_SIMPLE));
/*
          					^^^^
          				das sind 134 bei "simple"
*/
#else
          xPaperOut=(cxPaper-cxPaperOut)*100/(150-16*(nClipType==CLIP_CENTER_SIMPLE));
#endif
          xPaperOut-=(xPaperOut % 8);
          if (bVerbose)
            fprintf(stderr,"limiting X to %d / %d pixels\n",
            	xPaperOut,cxPaperOut);
         }
        if (cyPaper>CY_MAX_A4)
         {
          cyPaperOut=CY_MAX_A4;
          yPaperOut=(cyPaper-cyPaperOut)/3;
          /* yPaperOut-=(yPaperOut % 8); */
          if (bVerbose)
            fprintf(stderr,"limiting Y to %d / %d pixels\n",
            	yPaperOut,cyPaperOut);
         }
       break;
     case CLIP_FIX_BORDERS:
	/* fprintf(stderr,"Wir hatten: %d/%d",xPaperOut,yPaperOut); */
     	xPaperOut=nPCLResolution/5;
        xPaperOut-=(xPaperOut % 8);
     	yPaperOut=nPCLResolution/7; /* xxx */
	/* fprintf(stderr," und bekommen: %d/%d\n",xPaperOut,yPaperOut); */
     	break;
   }
 }
   
/*
 ******************************************************************
 * RotatePage()
 ******************************************************************
 */

static void Swap(int *x, int *y)
 {
  int n;
  n = *x; *x = *y; *y = n;
 }

int RotatePage(void)
 {
 	/**
 	Es muß erst ein neuer Puffer geholt werden,
 	dann wird er bitweise verfüllt, und am Ende
 	wird er aktueller Puffer.
 	*/
  long 			lcchNewLine=(cyPaper+7L)/8L;
  long 			lcchNewBuf=cxPaper*lcchNewLine;
  unsigned char		*pchNewBuf;
  int			x0,y0,x,y;
  BOOL	abitBuf[8][8];
  if (bVerbose)
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
  for (x0=0; x0<cxPaper; x0+=8)
    for (y0=0; y0<cyPaper; y0+=8)
     {
     	/**
     	Zuerst wird ein Quellkästchen geholt, mit Überlappungsschutz unten.
     	*/
       {
        unsigned char uch;
        int yMax=cyPaper-y0;
        if (yMax>8) yMax=8;
        for (y=0; y<8; y++)
         {
          if (y>yMax)
            uch=0;
          else
            uch=pchFullPage[(y0+y)*lcchFullPageLine+x0/8];
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
        x1=cyPaper-y0;
        y1=x0;
        for (y=0; y<8; y++) /* y ist nun vertikal! */
         {
          uch=0;
          for (x=0; x<8; x++)
            uch=(uch<<1)|abitBuf[y][7-x];
          if (y1<cxPaper)
            pchNewBuf[lcchNewLine*(y1+y)+(x1/8L)]=uch;
         }
       } /* write */
     } /* y0 */
   /* x0 */
#ifdef DIRECT_DUMP
  printf("P4\n%d %d\n",cyPaper,cxPaper);
  fwrite(pchNewBuf,lcchNewBuf,1,stdout);
  free(pchNewBuf);
#endif
  free(pchFullPage);
  pchFullPage=pchNewBuf;
  Swap(&cxPaper,&cyPaper);
  Swap(&xPaperOut,&yPaperOut);
  Swap(&cxPaperOut,&cyPaperOut);
  lcchFullPageLine=lcchNewLine;
  lcFullPageLines=cyPaper;
  return 0;
 }

/*
 ******************************************************************
 *                             main()
 ******************************************************************
 */

int main(int argc, char **argv)
 {
  int	i;
  InitGlobal();
  if (!SetTables())
   {
    fprintf(stderr,"Error translating tables!\n");
    exit(1);
   }
  i=GetOptions(argc,argv);
  if (i) return i;
  if (bVerbose)
    fprintf(stderr,"*** g4 decoder/encoder v" VERSION" by Marian Eichholz 1997 ***\n\n");
  if (bBacon)
   {
    iByte=WalkBacon(stdin); /* was hat es bloss mit dem iByte auf sich? */
    if (iByte!=256)
     {
      fprintf(stderr,"fatal: error reading BACON header!\n");
      exit(1);
     }
   }
  else
   {
    iByte=cchHeader;
    if (cchHeader)
     {
      int ch;
      if (bVerbose)
        fprintf(stderr,"skipping %d header bytes...\n",cchHeader);
      while (cchHeader && (1==fread(&ch,1,1,stdin)))
        cchHeader--;
     }
    if (cchHeader)
     {
      fprintf(stderr,"error skipping header");
      return 1;
     }
   }
   		/**
   		Initialisiere die Puffer und Arbeitsparameter.
   		*/
  if (bFullPage)
   {
    pchFullPage=calloc(1,lcchFullPageLine*lcFullPageLines);
    if (pchFullPage==NULL)
     {
      fprintf(stderr,"fatal: not enough memory\n");
      return 1;
     }
   }
  else
    pchFullPage=NULL;
  i=DecodePage();
  /*
     Mit den Rückkehrcode ist das so eine Sache.
    Normalerweise sind die Toplevelfunktionen 0=OK, 1=FAIL
    (um Codes ausscheiden zu können), aber Helper sind häufig
    0=FAIL. Das ist in der Tat unschön.
  */
  if (!i && bRotate) /* auf Rotation hin wird nicht gleitend kodiert, sondern
			in einem eigenen Pass. Pech. Das kostet Platz. */
   {
    i=RotatePage();
    if (!i) i=EncodePage();
   }
   	/**
   	Der Speicher wird freigegeben und andere Aufräumarbeiten
   	werden durchgeführt.
   	*/
  if (pchFullPage)
    free(pchFullPage);
  return i;
 }
/* $Extended$File$Info$
 */
