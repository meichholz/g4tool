/*
*
* $Author: eichholz $
* $Revision: 1.1 $
* $State: Exp $
* $Date: 2001/12/28 21:50:39 $
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
* <pcl.c>
*
* Die aktuelle Zeile wird hier zu PCL-Delta-Code übersetzt.
* Teile der Fileinitialisierung laufen auch hier.
*
* ******************************************************* */

#include "global.h"

/*
 ******************************************************************
 * WriteHeadPCL()
 ******************************************************************
 */

BOOL WriteHeadPCL(FILE *f)
 {
    	/* fprintf(f,"\x1B" "E"); */	/* Jobstart */
    	fprintf(f,"\x1B&l%dO",	/* Orientierung */
    		bLandscape ? 1 : 0 );
    	fprintf(f,
    		"\x1B&a0L"	/* Rand links weg */
    		"\x1B&l0E"	/* Rand oben weg */
    		"\x1B*t%dR"	/* Resolution */
    		"\x1B*r0F"	/* logische Seite */
    		"\x1B*r1A"	/* ab ? (start graphics) */
    		"\x1B*b1Y"	/* Seed to 0 */
    		"\x1B*b3M"	/* directly compress! */
    		, nPCLResolution
		);
  /* fprintf(stderr,"coding for %d dpi\n",nPCLResolution); */
  return TRUE;
 }
 
/*
 ******************************************************************
 * DumpPagePCL()
 ******************************************************************
 */

BOOL DumpPagePCL(FILE *f)
 {
 	/**
 	Diese Funktion ist im Grunde Dummy.
 	Sie fügt nur noch den Seitenabschluß
 	an die Ausgabe an.
 	*/
  long lcchLine=lcchFullPageLine;
  long liNextBlock=(cxPaper-1L)/8L;
  while (liNextBlock>=0)
   {
    int y;
    unsigned char *pu=pchFullPage+liNextBlock;
    for (y=0; y<cyPaper; y++)
     {
      /* printf(.....) */
      pu+=lcchLine;
     }
    liNextBlock--;
   }
  printf("\x1B*rC\f");
  return TRUE;
 }
 
/*
 ******************************************************************
 * FlushLine()
 *
 * Aus Geschwindigkeitsgründen kann der Code nicht besser
 * modularisiert werden.
 *
 * Zuerst wird der PCL-Encoder gezeigt.
 *
 ******************************************************************
 */

BOOL FlushLinePCL(FILE *f)	/* für Laserjet 300 DPI */
 {
  int	iWorkBit;
  int	iLastWorkBit;
  int	uch;
  int	cBits=0;
  int	iPrev,iCurrent;
  
  unsigned char	*pchRef, *pchCurrent, *pchPrev, *pchDelta;
  
  unsigned char achPCL[CCH_PCL_BUF];
  int	iPCL=0;
  BOOL	bRaw=(iLine==yPaperOut+1);
  		/* Die erste Zeile muß roh übertragen werden */
  if (iLine<=yPaperOut) return TRUE;	/* aber davor NICHTS! */
#ifdef SUPPRESS_DELTA_PCL
  bRaw=TRUE;
#endif
#ifdef DIRECT_DELTA_PCL
  bRaw=FALSE;
#endif

  	/**
  	Die Fundamentalparameter werden hier schon einmal
  	berechnet.
  	*/
#ifdef SUPPORT_FULL_PAGE
  iCurrent=iLine-1;
  iPrev=iLine-2;
#else
  iCurrent=(iLine-1) & 1;
  iPrev=(iLine-2) & 1;
#endif
  pchRef=pchCurrent=pchFullPage+lcchFullPageLine*iCurrent
  		+(xPaperOut/8);
  pchPrev=          pchFullPage+lcchFullPageLine*iPrev
  		+(xPaperOut/8);

  	/**
  	vorsichtshalber nochmal nicht-Kompression verlangen,
  	wenn die erste Zeile übertragen wird.
  	*/
  if (bRaw) fprintf(f,"\x1B*b0M");
	/**
	Zuerst wird die neue Zeile (in abitWork) zu Bytes verpackt,
	und in den Seitenpuffer geschrieben.
 	(ob es den noch braucht, ist nebensächlich).
 	Bei Rohdatenübertragung (die erste Zeile zumindest)
 	werden die Bytes auch gleich in den PCL-Ausgabepuffer geschrieben.
	*/
  iLastWorkBit=cxPaperOut+xPaperOut;
  for (iWorkBit=xPaperOut+1; iWorkBit<=iLastWorkBit; iWorkBit++) /* ohne Rand... */

   {
    /** Der bei einigen Quellen auftretende rechte Ranstreifen
        wird explizit geweißelt! */
    BOOL bBit=iWorkBit>cxPaper ? 0 : abitWork[iWorkBit];
    uch=(uch<<1) | bBit;
    if (++cBits == 8)
     {
      if (bRaw)
        achPCL[iPCL++]=uch;
      cBits=0;
      *pchCurrent++ = uch;
     }
   }
  /** padden, und Zeile abschließen */
  if (cBits)
   {
    uch=(uch<<(8-cBits));
    *pchCurrent=uch;
    if (bRaw) achPCL[iPCL++]=uch;
   }

	/**
	Wenn nicht im Rohformat geschrieben wird,
	müssen Deltasequenzen übertragen werden.
	Dabei wird der Inhalt der gerade berechneten Zeile mit dem
	der letzten Zeile verglichen.
	*/  
  if (!bRaw)
   {
    int iByte=0;
    int	iLastByte=(cxPaperOut+7)/8; /* hmmm... wg "+1" kann auch 8 sein */
    int	iCount;
    int iOffset;
    pchCurrent=pchRef;
    	/**
    	Bis zum Ende der Zeile wird immer das Paar durchlaufen:
    	Gleiche Bytes zählen, Differenzblock bestimmen
    	und das Ergebnis übertragen.
    	*/
    while (iByte < iLastByte)
     {
      		/**
		Zähle gleiche Bytes. Die Aufteilung des Ergebnisses
		wird später erledigt.
		*/
      iOffset=0;
      while (iByte < iLastByte && (*pchCurrent) == (*pchPrev))
       {
        pchCurrent++;
        pchPrev++;
        iByte++;
        iOffset++;
       }
      		/**
		Zähle ungleiche Bytes;
		*/
      iCount=0;			/* zu schreibende Differnzbytes */
      pchDelta = pchCurrent;	/* Start des nächsten Differenzblocks */
      while (iByte < iLastByte && (*pchCurrent) != (*pchPrev))
       {
        pchCurrent++;
        pchPrev++;
        iByte++;
        iCount++;
        	/**
        	Bei Überläufen wird ein Differenzblock
        	gedumpt.
        	
        	Der Offset muß dabei notfalls zusammengesetzt werden.
        	*/
        if (iCount==8)
         {
          int i;
          int iOut=iOffset;
          if (iOut>30)
            iOut=31;
          iOffset-=31;
          achPCL[iPCL++]=(0xE0u | iOut );
          while (iOffset>=255) /* böse Falle: es darf keine 255 übrigbleiben! */
           {
            achPCL[iPCL++]=0xFFu;
            iOffset-=255;
           }
          if (iOffset>=0)
            achPCL[iPCL++]=iOffset;
          for (i=0; i<8; i++)
            achPCL[iPCL++] = *pchDelta++;
          iCount=0;  
          iOffset=0;
         }
       } /* while unequal part */
       		/**
       		Die (Rest-) Differenz wird offsetrichtig ausgegeben.
       		Auch hier muß der Offset notfalls zusammengerechnet
       		werden.
       		*/
      if (iCount)
       {
        int iOut=iOffset;
        int i; /* Ups: (8.10.1998) Da wurde doch der I-Zähler zurückgesetzt! */
        if (iOut>30)
          iOut=31;
        iOffset-=31;
        achPCL[iPCL++]=((iCount-1)<<5) | iOut ;
        while (iOffset>=255)
         {
          achPCL[iPCL++]=0xFFu;
          iOffset-=255;
         }
        if (iOffset>=0)
          achPCL[iPCL++]=iOffset;
        for (i=0; i<iCount; i++)
          achPCL[iPCL++]=*pchDelta++;
       }
     } /* while Zeile noch nicht fertig */
   } /* if not raw format */
   
   	/**
   	Der gerade berechnete Puffer wird ausgegeben.
   	*/
  if (iPCL>CCH_PCL_BUF)
   {
    fprintf(stderr,"warning: pcl buffer overflowed in %d (%d)\n",iLine,iPCL);
   }
  if (bDebugPCL)
    fprintf(stderr,"debug: %d bytes in %d\n",iPCL,iLine);
  fprintf(f,"\x1B*b%dW",iPCL);	/* EC *b war letztes */
  fwrite(achPCL,iPCL,1,f);
  	/**
  	Nach der ersten Zeile wird auf Deltakompression
  	umgeschaltet.
  	*/
  if (bRaw)
    fprintf(f,"\x1B*b3M");
  return TRUE;
 }

