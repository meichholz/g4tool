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
     "\t -V : request version string\n" \
     "\t -H : this page\n" \
     ""
/*
 * InitGlobal()
 */

static void InitGlobal(THIS)
 {
  this->cxPaper=G4T_CX_MAX;
  this->cyPaper=G4T_CY_MAX;
  this->lcchFullPageLine=(G4T_CX_MAX+7L)/8L;
  this->nPCLResolution=300;
#ifdef SUPPORT_FULL_PAGE
  this->lcFullPageLines=G4T_CY_MAX;
#else
  this->lcFullPageLines=2;
#endif
 }

/*
 ******************************************************************
 * Walkbacon()
 ******************************************************************
 */

static int WalkBacon(THIS, FILE *f)
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
 ******************************************************************
 * GetOptions()
 ******************************************************************
 */

static int GetOptions(THIS,
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
      case 'b': this->bBacon=TRUE; break;
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
 *                             main()
 ******************************************************************
 */

int main(int argc, char **argv)
 {
  int	i,cchHeader;
  THIS = g4tInit();
  InitGlobal(this);
  if (!g4tSetTables(this))
   {
    fprintf(stderr,"Error translating tables!\n");
    exit(1);
   }
  cchHeader=0;
  i=GetOptions(this,&cchHeader,argc,argv);
  if (i) return i;
  if (this->bVerbose)
    fprintf(stderr,"*** g4 decoder/encoder v" VERSION" by Marian Eichholz 1997/2002 ***\n\n");
  if (this->bBacon)
   {
    this->iByte=WalkBacon(this,stdin); /* was hat es bloss mit dem this->iByte auf sich? */
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
  i=g4tDecodePage(this,stdout);
  /*
     Mit den Rückkehrcode ist das so eine Sache.
    Normalerweise sind die Toplevelfunktionen 0=OK, 1=FAIL
    (um Codes ausscheiden zu können), aber Helper sind häufig
    0=FAIL. Das ist in der Tat unschön.
  */
  if (!i && this->bRotate) /* auf Rotation hin wird nicht gleitend kodiert, sondern
			in einem eigenen Pass. Pech. Das kostet Platz. */
   {
    i=g4tRotatePage(this);
    if (i==G4T_EOK)
      i=g4tEncodePage(this,stdout);
   }
   	/**
   	Der Speicher wird freigegeben und andere Aufräumarbeiten
   	werden durchgeführt.
   	*/
  g4tExit(this);
  return i;
 }

