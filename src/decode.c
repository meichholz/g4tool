/*
*
* $Author: eichholz $
* $Revision: 1.3 $
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
* <decode.c>
*
* Hier wird eine G4-Zeile mit Hilfe des externen Gedächtnisses
* nach aBitWork dekodiert.
*
* g4tSetTables(this,) macht Initialisierung der Tabellen.
* GetLine() holt Codes und übersetzt.
*
* Die Laufsteuerung erfolgt darüberhinaus extern.
*
* ******************************************************* */

/**
	Die Indizes beschreiben die virtuellen Pixel nach T.4-Spec.
	Das bedeutet, daß das erste Pixel immer(!) weiß ist [Index 0],
	und daß die Zeile von 1..this->cxPaper beschrieben wird.
	
	Die Variable "this->a" beschreibt dabei die jeweils zuletzt dekodierte
	Position, initial also 0 (und WHITE).
	
	Die letzte Spalte (this->cxPaper) muß also auch kodiert werden (z.B. mit V(0).
	
	In (this->a) steht demnach auch stets die lette kodierte Farbe zur Verfügung
	(als Lookahead für die H-Codes beispielsweise).
	
*/

#include "global.h"

/*
 ******************************************************************
 * GetBit()
 ******************************************************************
 */

static int GetBit(FILE *f)
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

static BOOL SkipBits(FILE *f, int cBits)
 {
  BOOL bEOF=FALSE;
  while (cBits-- && !bEOF)
    bEOF=(GetBit(f)<0);
  return !bEOF;
 }

/*
 ******************************************************************
 * SetBCodes()
 ******************************************************************
 */

static void SetBCodes(int a0, int *pb1, int *pb2)
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

static void Vertical(int nType, int nOffset)
 {
  int	a0,b1;
  int	i;
  BOOL	bValue;
  int nTotalOffset=((nType) ? 1 : -1)*nOffset; /* nType:1=R */
  a0=this->a;
  SetBCodes(this->a,&b1,NULL);
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

static void Horizontal(int colFirst,int *cRuns)
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

static void Passcode(void)
 {
  int dummy,b2,i;
  int nColor;
  SetBCodes(this->a,&dummy,&b2);
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

static BOOL GetSymbol(FILE *f, int *pnSymbol, BOOL *pbEOF)
 {
  long		ulSymbol=1;
  BOOL		bWaiting=FALSE;
  static BOOL	bLastEOL=FALSE;
  		/**
  		Ein Symbol wird identifiziert. Nur Basissymbole!
  		Die Schleife läuft bis EOF oder Fund.
  		Bei Symbolüberlauf wird auf EOL-Entdeckung
  		geschaltet.
  		*/
  *pbEOF=FALSE;
  do
   {
    int nBit=GetBit(f);
    if (nBit<0)
     {
      if (this->iLine<this->cyPaper || !this->bNoCheckEOL)
        fprintf(stderr,"unexpected EOF at byte %d/%X\n",this->iByte,iByte);
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

static BOOL GetColorRun(FILE *f, int iColor, int *pcRuns, BOOL *pbEOF)
 {
  long		ulSymbol=1;
  BOOL		bWaiting=FALSE;
  		/**
  		Systematik fast wie GetSymbol.
  		Nur anderes Suchverfahren.
  		*/
  *pbEOF=FALSE;
  *pcRuns=0;
  
  do
   {
    int nBit=GetBit(f);
    /* fprintf(stderr,"%d",nBit); */
    if (nBit<0)
     {
      fprintf(stderr,"unexpected EOF at byte %d/%X\n",this->iByte,iByte);
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

BOOL GetLine(FILE *f)
 {
  int nSymbol;
  BOOL	bError,bEOF;
  BOOL	bBreak=FALSE;
  int	acCols[2];
  do
   {
    if (this->a<0 || this->a>this->cxPaper)
     {
      fprintf(stderr,"oops: parameter runaway!");
      return FALSE;
     }
    bError=!GetSymbol(f,&nSymbol,&bEOF);
    if (!bEOF)
     {
      switch (nSymbol)
       {
        case SYM_P:	Passcode(); break;
        case SYM_V0:	Vertical(1,0); break;
        case SYM_VR1:	Vertical(1,1); break;
        case SYM_VR2:	Vertical(1,2); break;
        case SYM_VR3:	Vertical(1,3); break;
        case SYM_VL1:	Vertical(0,1); break;
        case SYM_VL2:	Vertical(0,2); break;
        case SYM_VL3:	Vertical(0,3); break;
        case SYM_EXT:
		fprintf(stderr,"<EXT> detected at %d/%X: not supported.\n",
				this->iByte,iByte);
		bEOF=!SkipBits(f,3);
        	break;
        case SYM_H:
         	 {
		  int colFirst=this->abitWork[this->a]; /* +1? */
		  		/*!this->abitWork[this->a];*/
        	  bError=!GetColorRun(f,colFirst,acCols,&bEOF);
        	  
        	  if (!bError)
         	    bError=!GetColorRun(f,!colFirst,acCols+1,&bEOF);
         	  if (!bError)
         	    Horizontal(colFirst,acCols);
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
 * g4tSetTables(this,)
 ******************************************************************
 */

	/**
	Zuerst wird die Dualkodierung der Lauflängen-Codes
	aus den handeingegebenen Tabellen
	berechnet.
	
	Der Code wird nach Codewort aufsteigend einsortiert.
	*/
BOOL g4tSetTables(this,void)
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
 * g4tDecodePage(this,)
 ******************************************************************
 */

int g4tDecodePage(this,void)
 {
  int i;
  BOOL bAbort=FALSE;
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
    g4tWriteHeader(this,stdout);
  this->iLine=1;
  this->a=0;
  this->abitRef=this->aabitBuffers[1];
  this->abitWork=this->aabitBuffers[0];
  this->iBuffer=0;
  g4tCenterPage(this,);
  for (i=0; i<this->cxPaper+3; this->abitWork[i]=this->abitRef[i]=WHITE) i++;
  while (!bAbort && GetLine(stdin))
   {
    if (this->bDebugVerbose)
      fprintf(stderr,"setting line %d...\n",this->iLine);
      		/**
      		Erst wird die neu berechnete Zeile ausgeworfen,
      		und erst dann werden die Puffer umgeschaltet.
      		So kann ein Flusher auch die Referenzzeile
      		mitverwenden.
      		*/
    if (this->iLine<CY_MAX)
      if (this->bRotate)
        g4tEncodePageLine(this,);
      else
        g4tFlushLine(this,stdout);
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
    g4tClosePage(this,stdout);   
   	/**
   	Die Anzahl der Zeilen wird geprüft. Die wird bei einem
   	Formatfehler meist abweichen.
   	*/
  if (this->cyPaper+1!=this->iLine)
    if (!this->bNoLineWarnings)
     {
      fprintf(stderr,"got %d lines instead of %d\n",this->iLine-1,this->cyPaper);
      return 1;
     }
  return 0;
 }

/* $Extended$File$Info$
 */
