/* *******************************************************
*
* <decode.c>
*
* A g4 coded line is decoded to <aBitWork> through an external memory.
*
* g4tSetTables() initialises the tables.
* GetLine() gets new tokens and translates them.
*
* run control is external.
*
* ******************************************************* */

/**
indices describe "virtual pixels" according to T.4.
the first pixel is defined WHITE (index 0).
the real line is in 1..this->cxPaper.
this->a is the last decoded position and starts with 0.

thus, the last column [this->cxPaper] MUST be decoded (with V(0)).

this->a has the last coded color, used as look-ahead for the H codes.

*/

#include "global.h"

/*
 ******************************************************************
 * GetBit()
 ******************************************************************
 */

static int GetBit(THIS, FILE *f)
 {
  static 		int cAvailBits=0;
  static unsigned	char chAkku;
  int			nResult;
  if (!cAvailBits)
   {
    if (1!=fread(&chAkku,1,1,f))
      return -1;
    cAvailBits=8;
    this->iByte++;
   }
  cAvailBits--; 
  nResult=(chAkku & 0x80)>>7;
  chAkku=(chAkku<<1);
  return nResult;
 }

/*
 ******************************************************************
 * SkipBits()
 ******************************************************************
 */

static G4T_BOOL SkipBits(THIS, FILE *f, int cBits)
 {
  G4T_BOOL bEOF=FALSE;
  while (cBits-- && !bEOF)
    bEOF=(GetBit(this, f)<0);
  return !bEOF;
 }

/*
 ******************************************************************
 * SetBCodes()
 ******************************************************************
 */

static void SetBCodes(THIS, int a0, int *pb1, int *pb2)
 {
  int	i=a0+1; /* fuer schnellerer Rechnung!!! */
  int color0=this->abitWork[a0];
  int oldrefcol=this->abitRef[a0];
  	/**
  	Die korrekte Behandlug ndes Anfangs ist wegen der schwierigen
  	Definition des "changing Elements" etwas heikel.
  	
  	Der folgende Spezialfall scheint zu funktionieren...
  	*/
#define xPATCH_B_CODE  	
#ifdef PATCH_B_CODE  	
  if (!a0 && oldrefcol)
   {
    *pb1=1; i++;
    /* color0=oldrefcol; */
   }
  else
#endif  
   {
    if (i>this->cxPaper)
     {
      *pb1=this->cxPaper+1;
      if (pb2) *pb2=this->cxPaper+1; /* zeitlich völlig unkritisch! */
      return;
     }
   	/** changing element aufsuchen */
    while ((i<=this->cxPaper) && (this->abitRef[i]==oldrefcol)) i++;
    	/** Gleiche Farben übergehen */
    while ((i<=this->cxPaper) && (this->abitRef[i]==color0)) i++;
    *pb1=i;
   }
  if (pb2)
   {
    	/* Ungleiche Farben übergehen. */
    while ((i<=this->cxPaper) && (this->abitRef[i]!=color0)) i++;
    *pb2=i;
   }
 }

/*
 ******************************************************************
 * Vertical()
 ******************************************************************
 */

static void Vertical(THIS, int nType, int nOffset)
 {
  int	a0,b1;
  int	i;
  G4T_BOOL	bValue;
  int nTotalOffset=((nType) ? 1 : -1)*nOffset; /* nType:1=R */
  a0=this->a;
  SetBCodes(this,this->a,&b1,NULL);
  bValue=this->abitWork[this->a];
  this->a=b1+nTotalOffset;
  for (i=a0; i<this->a; i++)
    this->abitWork[i]=bValue;
  this->abitWork[this->a]=!bValue;
  if (this->bDebugVerbose)
    fprintf(stderr,"<V%c(%d)> code detected (%d->%d)\n",
    	(nType ? 'R' : 'L'),
    	nOffset, a0,this->a);
 }

/*
 ******************************************************************
 * Horizontal()
 ******************************************************************
 */

static void Horizontal(THIS, int colFirst,int *cRuns)
 {
  int iColor=0;
  static char achCols[]="WB";
  if (this->bDebugVerbose)
   {
    fprintf(stderr,"<H(%c%c,%d,%d) > found.\n",
    	achCols[colFirst],achCols[!colFirst],cRuns[0],cRuns[1]);
   }
  for (iColor=0; iColor<2; iColor++)
   {
    int a0=this->a;
    int i;
    if (!this->a) { this->a++; a0++; }
    this->a+=cRuns[iColor];
    for (i=a0; i<this->a; i++)
      this->abitWork[i]=colFirst;
    colFirst=!colFirst;
   }
  this->abitWork[this->a]=colFirst;
 }

/*
 ******************************************************************
 * Passcode()
 ******************************************************************
 */

static void Passcode(THIS)
 {
  int dummy,b2,i;
  int nColor;
  SetBCodes(this,this->a,&dummy,&b2);
  	/**
  	Bis zum neuen b2 wird das Feld in der Farbe (a0) verfüllt.
  	Das neue (a0) wird gleich mitverfüllt.
  	*/
  nColor=this->abitWork[this->a];
  for (i=this->a+1; i<=b2; i++)
    this->abitWork[i]=nColor;
  if (this->bDebugVerbose)
    fprintf(stderr,"P for color %d at line %d (%d->%d)\n",nColor,this->iLine,this->a,b2);
  this->a=b2;
 }
 
/*
 ******************************************************************
 * GetSymbol()
 ******************************************************************
 */

static G4T_BOOL GetSymbol(THIS, FILE *f, int *pnSymbol, G4T_BOOL *pbEOF)
 {
  long		ulSymbol=1;
  G4T_BOOL		bWaiting=FALSE;
  static G4T_BOOL	bLastEOL=FALSE;
  		/**
  		Ein Symbol wird identifiziert. Nur Basissymbole!
  		Die Schleife läuft bis EOF oder Fund.
  		Bei Symbolüberlauf wird auf EOL-Entdeckung
  		geschaltet.
  		*/
  *pbEOF=FALSE;
  do
   {
    int nBit=GetBit(this,f);
    if (nBit<0)
     {
      if (this->iLine<this->cyPaper || !this->bNoCheckEOL)
        fprintf(stderr,"unexpected EOF at byte %d/%X\n",
		this->iByte,this->iByte);
      *pbEOF=TRUE;
      return FALSE;
     }
    ulSymbol=(ulSymbol<<1)| !!nBit;
    switch (ulSymbol)
     {
      case SYM_EOL:
      		if (bLastEOL)
      		  *pnSymbol=SYM_EOFB;
      		else
      		  *pnSymbol = ulSymbol;
      		bLastEOL=TRUE;
      		return !bWaiting;
      case SYM_P:
      case SYM_V0:
      case SYM_VR1:
      case SYM_VR2:
      case SYM_VR3:
      case SYM_VL1:
      case SYM_VL2:
      case SYM_VL3:
      case SYM_EXT:
      case SYM_H:
      		if (!bWaiting)
      		 {
      		  *pnSymbol = ulSymbol;
      		  return TRUE;
      		 }
      		break;
     }
    if (bWaiting && nBit)
      ulSymbol=1;
    if ((ulSymbol>0x40000000) && !bWaiting)
     {
      fprintf(stderr,"symbol overflow in line %d,"
      		" byte %d (%lx), skipping to EOL\n",
      		this->iLine,this->iByte,ulSymbol);
      bWaiting=TRUE;
      bLastEOL=FALSE;
      ulSymbol=1;
     }
   } while (TRUE);
  // return FALSE;
 }

/*
 ******************************************************************
 * GetColorRun()
 ******************************************************************
 */

static G4T_BOOL GetColorRun(THIS, FILE *f, int iColor, int *pcRuns, G4T_BOOL *pbEOF)
 {
  long		ulSymbol=1;
  G4T_BOOL		bWaiting=FALSE;
  		/**
  		Systematik fast wie GetSymbol.
  		Nur anderes Suchverfahren.
  		*/
  *pbEOF=FALSE;
  *pcRuns=0;
  
  do
   {
    int nBit=GetBit(this,f);
    /* fprintf(stderr,"%d",nBit); */
    if (nBit<0)
     {
      fprintf(stderr,"unexpected EOF at byte %d/%X\n",
	      this->iByte,this->iByte);
      *pbEOF=TRUE;
      return FALSE;
     }
    ulSymbol=(ulSymbol<<1)| !!nBit;
    if (ulSymbol==SYM_EOL)
     {
      if (bWaiting)
       {
        *pcRuns=-1;
        return FALSE;
       }
      else
       {
        fprintf(stderr,"Unexpected EOL at line %d\n",this->iLine);
        *pcRuns=-1;
        return FALSE;
       }
     }
    else
     {
      TRunSpec	*prs=this->arsRuns[iColor];
      		/*
      		Die Bitmasken werden komplett für jede
      		Bitkombination durchgehechelt und die gefundenen
      		Makeupcodes aufaddiert.
      		Verbesserung: Binärsuche.
      		*/
      int i0=0;
      int i1=this->acRuns[iColor]-1;
      while (i0<=i1)
       {
        int i=(i0+i1)/2;
        unsigned int us=prs[i].bitMask;
        if (this->bDebugShowRunComp)
          fprintf(stderr,"comparing: (%d,%d,%d) %04lX and %04Xd\n",
		i0,i,i1, ulSymbol,us);
        if (us==ulSymbol)
         {
          if (this->bDebugVerbose)
            fprintf(stderr,"Gefunden: %u for %lxh\n",
            		(unsigned int)prs[i].cPixel,
          		(long)ulSymbol);
          (*pcRuns)+=prs[i].cPixel;
          ulSymbol=1;
          	/**
          	Bei Termcode wird verlassen, sonst nur die
          	Suchschleife abgebrochen.
          	*/
          if (prs[i].cPixel < 64)
            return TRUE;
          else
            break;
         }
        else
          if (us<ulSymbol)
            i0=i+1;
          else
            i1=i-1;
       } // while prs
     }
    if (bWaiting && nBit)
      ulSymbol=1;
    if ((ulSymbol>0x40000000))
     {
      fprintf(stderr,"symbol overflow in line %d,"
      		" byte %d (%lx), phase %c, skipping to EOL\n",
      		this->iLine,this->iByte,ulSymbol,
      		(iColor==WHITE) ? 'W' : ((iColor==BLACK) ? 'B' : '?'));
      		/**
      		Es wird beim Fehler nicht lange gesucht, sondern gleich
      		abgebrochen, dnen es nutzt nix.
      		*/
      *pbEOF=TRUE;
      return FALSE;
     }
   } while (TRUE);
 }

/*
 ******************************************************************
 * GetLine()
 ******************************************************************
 */

static G4T_BOOL GetLine(THIS, FILE *f)
 {
  int nSymbol;
  G4T_BOOL	bError,bEOF;
  G4T_BOOL	bBreak=FALSE;
  int	acCols[2];
  do
   {
    if (this->a<0 || this->a>this->cxPaper)
     {
      fprintf(stderr,"oops: parameter runaway!");
      return FALSE;
     }
    bError=!GetSymbol(this, f, &nSymbol, &bEOF);
    if (!bEOF)
     {
      switch (nSymbol)
       {
        case SYM_P:	Passcode(this); break;
        case SYM_V0:	Vertical(this,1,0); break;
        case SYM_VR1:	Vertical(this,1,1); break;
        case SYM_VR2:	Vertical(this,1,2); break;
        case SYM_VR3:	Vertical(this,1,3); break;
        case SYM_VL1:	Vertical(this,0,1); break;
        case SYM_VL2:	Vertical(this,0,2); break;
        case SYM_VL3:	Vertical(this,0,3); break;
        case SYM_EXT:
		fprintf(stderr,"<EXT> detected at %d/%X: not supported.\n",
				this->iByte,this->iByte);
		bEOF=!SkipBits(this,f,3);
        	break;
        case SYM_H:
         	 {
		  int colFirst=this->abitWork[this->a]; /* +1? */
		  		/*!this->abitWork[this->a];*/
        	  bError=!GetColorRun(this, f,colFirst,acCols,&bEOF);
        	  
        	  if (!bError)
         	    bError=!GetColorRun(this,f,!colFirst,acCols+1,&bEOF);
         	  if (!bError)
         	    Horizontal(this,colFirst,acCols);
         	 }
        	break;
        case SYM_EOFB:
        	if (this->bDebugVerbose)
        	  fprintf(stderr,"<EOFB> detected.\n");
        	bEOF=TRUE;
        	this->iLine--;
        	break;
        case SYM_EOL:
         	this->iLine--;
        	if (this->bDebugVerbose)
        	  fprintf(stderr,"<EOL> detected.\n");
        	if (bError)
        	  bEOF=TRUE;
		break;        	
        default:
        	fprintf(stderr,"unknown code...\n");
       }
     }
    if (this->a>this->cxPaper) /* auch die letzte Spalte muß kodiert sein!!! */
      bBreak=TRUE;
   } while (!bEOF && !bBreak &&
   		(nSymbol!=SYM_EOL) && (nSymbol!=SYM_EOFB));
  return !bEOF;
 }

/*
 ******************************************************************
 * g4tSetTables(...)
 ******************************************************************
 */

	/**
	Zuerst wird die Dualkodierung der Lauflängen-Codes
	aus den handeingegebenen Tabellen
	berechnet.
	
	Der Code wird nach Codewort aufsteigend einsortiert.
	*/
G4T_BOOL g4tSetTables(THIS)
 {
  int iColor, iRun;
  for (iColor=WHITE; iColor<=BLACK; iColor++)
   {
    int i;
    		/**
    		Zuallererst wird die Tabelle gelöscht.
    		*/
    for (iRun=0; iRun<CRUNTABLE; iRun++)
      this->arsRuns[iColor][iRun].bitMask=0;
      		/**
      		Dann erst geht es in die Mustertabelle.
      		*/
    for (	iRun=0;
    		arstSpecs[iColor][iRun].szBitMask
    			&& iRun<CRUNTABLE;
    		iRun++)
     {
      int	cBits=0;
      unsigned short usWord=1;
      int	iTarget=iRun;
      		/**
      		Erst mal wird das Codewort, vorn durch eine
      		pivotierende 1 ergänzt, bestimmt.
      		*/
      char *pch=arstSpecs[iColor][iRun].szBitMask;
      while (cBits<16 && *pch)
       {
        usWord=(usWord<<1) | (*pch=='1');
        pch++;
        cBits++;
       }
      		/**
      		Dann wird für die "straight insertion strategy"
      		der Zielindex bestimmt.
      		*/
      for 	(iTarget=0;
      		this->arsRuns[iColor][iTarget].bitMask
      		&& this->arsRuns[iColor][iTarget].bitMask<usWord;
      		iTarget++);
      		/**
      		Die oben liegenden Elemente werden hochgeschoben.
      		*/
      for (i=CRUNTABLE-1; i>iTarget; i--)
        this->arsRuns[iColor][i]=this->arsRuns[iColor][i-1];
        	/**
        	Dann erst wird das neue Element eingeschrieben.
        	*/
      this->arsRuns[iColor][iTarget].cPixel=arstSpecs[iColor][iRun].cPixel;
      this->arsRuns[iColor][iTarget].bitMask=usWord;
      if (*pch)
       {
        fprintf(stderr,"Fehler in color %d, runlength %d.\n",
        		iColor,arstSpecs[iColor][iRun].cPixel);
        return FALSE;
       }
     }
    if (iRun==CRUNTABLE)
     {
      fprintf(stderr,"Lookuptable overflow!\n");
      exit(2);
     }
    this->acRuns[iColor]=iRun;
 #ifdef DUMP_SPECS
    for (i=0; i<CRUNTABLE; i++)
      fprintf(stderr,"table %d, code=%04X, length=%d\n",
      	iColor,this->arsRuns[iColor][i].bitMask,this->arsRuns[iColor][i].cPixel);
 #endif
   }
  return TRUE;
 }

/*
 ******************************************************************
 * g4tDecodePage(...)
 ******************************************************************
 */

G4T_RC g4tDecodePage(THIS, FILE *f)
 {
  int i;
  G4T_BOOL bAbort=FALSE;
  	/**
  	Der eigentliche Dump wird nur dann gleitend ausgeführt,
  	wenn nicht rotiert wird.
  	
  	In diesem Fall muß explizit ein neuer Puffer angelegt und
  	verfüllt werden.

	return : 0 auf ok, 1 bei Fehler.
  	*/
  if (this->bVerbose)
    fprintf(stderr,"decoding...\n");
  if (!this->bRotate)
    g4tWriteHeader(this,f);
  this->iLine=1;
  this->a=0;
  this->abitRef=this->aabitBuffers[1];
  this->abitWork=this->aabitBuffers[0];
  this->iBuffer=0;
  g4tCenterPage(this);
  for (i=0;
       i<this->cxPaper+3;
       this->abitWork[i]=this->abitRef[i]=WHITE)
    i++;
  while (!bAbort && GetLine(this,stdin))
   {
    if (this->bDebugVerbose)
      fprintf(stderr,"setting line %d...\n",this->iLine);
      		/**
      		Erst wird die neu berechnete Zeile ausgeworfen,
      		und erst dann werden die Puffer umgeschaltet.
      		So kann ein Flusher auch die Referenzzeile
      		mitverwenden.
      		*/
    if (this->iLine<G4T_CY_MAX)
      if (this->bRotate)
        g4tEncodePageLine(this);
      else
        g4tFlushLine(this,f);
    this->iBuffer=(this->iBuffer+1) & 1;
    this->abitRef=this->abitWork;
    this->abitWork=this->aabitBuffers[this->iBuffer];
    this->iLine++;
    this->a=0;
    /**
    Die folgende Löschung im Schreibpuffer ist eigentlich merkwürdig
    deutet auf ein Designproblem der Aktionsprozeduren
    hin (Zwang zum Color-Lookahead).
    
    Ein Schwarz-Start wird durch VL(0) kodiert.
    */
    this->abitWork[0]=WHITE; /*this->abitRef[1];*/ /* nächste Farbe ist Weiß */
    this->abitRef[0]=WHITE;
    if (this->iLine>this->cyPaper && this->bNoCheckEOL) bAbort=TRUE;
   }
  if (!this->bRotate)
    g4tClosePage(this,f);   
   	/**
   	Die Anzahl der Zeilen wird geprüft. Die wird bei einem
   	Formatfehler meist abweichen.
   	*/
  if (this->cyPaper+1!=this->iLine)
    if (!this->bNoLineWarnings)
     {
      fprintf(stderr,"got %d lines instead of %d\n",this->iLine-1,this->cyPaper);
      return G4T_EGENERAL;
     }
  return G4T_EOK;
 }

/* $Extended$File$Info$
 */
