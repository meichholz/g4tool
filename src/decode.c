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
* <decode.c>
*
* Hier wird eine G4-Zeile mit Hilfe des externen Gedächtnisses
* nach aBitWork dekodiert.
*
* SetTables() macht Initialisierung der Tabellen.
* GetLine() holt Codes und übersetzt.
*
* Die Laufsteuerung erfolgt darüberhinaus extern.
*
* ******************************************************* */

/**
	Die Indizes beschreiben die virtuellen Pixel nach T.4-Spec.
	Das bedeutet, daß das erste Pixel immer(!) weiß ist [Index 0],
	und daß die Zeile von 1..cxPaper beschrieben wird.
	
	Die Variable "a" beschreibt dabei die jeweils zuletzt dekodierte
	Position, initial also 0 (und WHITE).
	
	Die letzte Spalte (cxPaper) muß also auch kodiert werden (z.B. mit V(0).
	
	In (a) steht demnach auch stets die lette kodierte Farbe zur Verfügung
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
    iByte++;
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
  int color0=abitWork[a0];
  int oldrefcol=abitRef[a0];
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
    if (i>cxPaper)
     {
      *pb1=cxPaper+1;
      if (pb2) *pb2=cxPaper+1; /* zeitlich völlig unkritisch! */
      return;
     }
   	/** changing element aufsuchen */
    while ((i<=cxPaper) && (abitRef[i]==oldrefcol)) i++;
    	/** Gleiche Farben übergehen */
    while ((i<=cxPaper) && (abitRef[i]==color0)) i++;
    *pb1=i;
   }
  if (pb2)
   {
    	/* Ungleiche Farben übergehen. */
    while ((i<=cxPaper) && (abitRef[i]!=color0)) i++;
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
  a0=a;
  SetBCodes(a,&b1,NULL);
  bValue=abitWork[a];
  a=b1+nTotalOffset;
  for (i=a0; i<a; i++)
    abitWork[i]=bValue;
  abitWork[a]=!bValue;
  if (bDebugVerbose)
    fprintf(stderr,"<V%c(%d)> code detected (%d->%d)\n",
    	(nType ? 'R' : 'L'),
    	nOffset, a0,a);
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
  if (bDebugVerbose)
   {
    fprintf(stderr,"<H(%c%c,%d,%d) > found.\n",
    	achCols[colFirst],achCols[!colFirst],cRuns[0],cRuns[1]);
   }
  for (iColor=0; iColor<2; iColor++)
   {
    int a0=a;
    int i;
    if (!a) { a++; a0++; }
    a+=cRuns[iColor];
    for (i=a0; i<a; i++)
      abitWork[i]=colFirst;
    colFirst=!colFirst;
   }
  abitWork[a]=colFirst;
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
  SetBCodes(a,&dummy,&b2);
  	/**
  	Bis zum neuen b2 wird das Feld in der Farbe (a0) verfüllt.
  	Das neue (a0) wird gleich mitverfüllt.
  	*/
  nColor=abitWork[a];
  for (i=a+1; i<=b2; i++)
    abitWork[i]=nColor;
  if (bDebugVerbose)
    fprintf(stderr,"P for color %d at line %d (%d->%d)\n",nColor,iLine,a,b2);
  a=b2;
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
      if (iLine<cyPaper || !bNoCheckEOL)
        fprintf(stderr,"unexpected EOF at byte %d/%X\n",iByte,iByte);
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
      		iLine,iByte,ulSymbol);
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
      fprintf(stderr,"unexpected EOF at byte %d/%X\n",iByte,iByte);
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
        fprintf(stderr,"Unexpected EOL at line %d\n",iLine);
        *pcRuns=-1;
        return FALSE;
       }
     }
    else
     {
      TRunSpec	*prs=arsRuns[iColor];
      		/*
      		Die Bitmasken werden komplett für jede
      		Bitkombination durchgehechelt und die gefundenen
      		Makeupcodes aufaddiert.
      		Verbesserung: Binärsuche.
      		*/
      int i0=0;
      int i1=acRuns[iColor]-1;
      while (i0<=i1)
       {
        int i=(i0+i1)/2;
        unsigned int us=prs[i].bitMask;
        if (bDebugShowRunComp)
          fprintf(stderr,"comparing: (%d,%d,%d) %04lX and %04Xd\n",
		i0,i,i1, ulSymbol,us);
        if (us==ulSymbol)
         {
          if (bDebugVerbose)
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
      		iLine,iByte,ulSymbol,
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
    if (a<0 || a>cxPaper)
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
				iByte,iByte);
		bEOF=!SkipBits(f,3);
        	break;
        case SYM_H:
         	 {
		  int colFirst=abitWork[a]; /* +1? */
		  		/*!abitWork[a];*/
        	  bError=!GetColorRun(f,colFirst,acCols,&bEOF);
        	  
        	  if (!bError)
         	    bError=!GetColorRun(f,!colFirst,acCols+1,&bEOF);
         	  if (!bError)
         	    Horizontal(colFirst,acCols);
         	 }
        	break;
        case SYM_EOFB:
        	if (bDebugVerbose)
        	  fprintf(stderr,"<EOFB> detected.\n");
        	bEOF=TRUE;
        	iLine--;
        	break;
        case SYM_EOL:
         	iLine--;
        	if (bDebugVerbose)
        	  fprintf(stderr,"<EOL> detected.\n");
        	if (bError)
        	  bEOF=TRUE;
		break;        	
        default:
        	fprintf(stderr,"unknown code...\n");
       }
     }
    if (a>cxPaper) /* auch die letzte Spalte muß kodiert sein!!! */
      bBreak=TRUE;
   } while (!bEOF && !bBreak &&
   		(nSymbol!=SYM_EOL) && (nSymbol!=SYM_EOFB));
  return !bEOF;
 }

/*
 ******************************************************************
 * SetTables()
 ******************************************************************
 */

	/**
	Zuerst wird die Dualkodierung der Lauflängen-Codes
	aus den handeingegebenen Tabellen
	berechnet.
	
	Der Code wird nach Codewort aufsteigend einsortiert.
	*/
BOOL SetTables(void)
 {
  int iColor, iRun;
  for (iColor=WHITE; iColor<=BLACK; iColor++)
   {
    int i;
    		/**
    		Zuallererst wird die Tabelle gelöscht.
    		*/
    for (iRun=0; iRun<CRUNTABLE; iRun++)
      arsRuns[iColor][iRun].bitMask=0;
      		/**
      		Dann erst geht es in die Mustertabelle.
      		*/
    for (	iRun=0;
    		arstSpecs[iColor][iRun].szBitMask
    			&& iRun<CRUNTABLE;
    		iRun++)
     {
      int	cBits=0;
      USHORT	usWord=1;
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
      		arsRuns[iColor][iTarget].bitMask
      		&& arsRuns[iColor][iTarget].bitMask<usWord;
      		iTarget++);
      		/**
      		Die oben liegenden Elemente werden hochgeschoben.
      		*/
      for (i=CRUNTABLE-1; i>iTarget; i--)
        arsRuns[iColor][i]=arsRuns[iColor][i-1];
        	/**
        	Dann erst wird das neue Element eingeschrieben.
        	*/
      arsRuns[iColor][iTarget].cPixel=arstSpecs[iColor][iRun].cPixel;
      arsRuns[iColor][iTarget].bitMask=usWord;
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
    acRuns[iColor]=iRun;
 #ifdef DUMP_SPECS
    for (i=0; i<CRUNTABLE; i++)
      fprintf(stderr,"table %d, code=%04X, length=%d\n",
      	iColor,arsRuns[iColor][i].bitMask,arsRuns[iColor][i].cPixel);
 #endif
   }
  return TRUE;
 }

/*
 ******************************************************************
 * DecodePage()
 ******************************************************************
 */

int DecodePage(void)
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
  if (bVerbose)
    fprintf(stderr,"decoding...\n");
  if (!bRotate)
    WriteHeader(stdout);
  iLine=1;
  a=0;
  abitRef=aabitBuffers[1];
  abitWork=aabitBuffers[0];
  iBuffer=0;
  CenterPage();
  for (i=0; i<cxPaper+3; abitWork[i]=abitRef[i]=WHITE) i++;
  while (!bAbort && GetLine(stdin))
   {
    if (bDebugVerbose)
      fprintf(stderr,"setting line %d...\n",iLine);
      		/**
      		Erst wird die neu berechnete Zeile ausgeworfen,
      		und erst dann werden die Puffer umgeschaltet.
      		So kann ein Flusher auch die Referenzzeile
      		mitverwenden.
      		*/
    if (iLine<CY_MAX)
      if (bRotate)
        EncodePageLine();
      else
        FlushLine(stdout);
    iBuffer=(iBuffer+1) & 1;
    abitRef=abitWork;
    abitWork=aabitBuffers[iBuffer];
    iLine++;
    a=0;
    /**
    Die folgende Löschung im Schreibpuffer ist eigentlich merkwürdig
    deutet auf ein Designproblem der Aktionsprozeduren
    hin (Zwang zum Color-Lookahead).
    
    Ein Schwarz-Start wird durch VL(0) kodiert.
    */
    abitWork[0]=WHITE; /*abitRef[1];*/ /* nächste Farbe ist Weiß */
    abitRef[0]=WHITE;
    if (iLine>cyPaper && bNoCheckEOL) bAbort=TRUE;
   }
  if (!bRotate)
    ClosePage(stdout);   
   	/**
   	Die Anzahl der Zeilen wird geprüft. Die wird bei einem
   	Formatfehler meist abweichen.
   	*/
  if (cyPaper+1!=iLine)
    if (!bNoLineWarnings)
     {
      fprintf(stderr,"got %d lines instead of %d\n",iLine-1,cyPaper);
      return 1;
     }
  return 0;
 }

/* $Extended$File$Info$
 */
