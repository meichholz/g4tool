/*
* g4tool
*
* transcode a G4 coded facsimile (with header) to a PPM/PCL/TIFF
*
* (C) Marian Matthias Eichholz 1st Mai 1997
*     reworked for libg4tool February 2002
*
*/

/* Revisionsgeschichte siehe <Revision> */

#ifndef H_GLOBAL
#define H_GLOBAL

#if HAVE_CONFIG_H
#	include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "g4tool.h"

#define TRUE    1
#define FALSE   0

#define	BLACK	1
#define WHITE	0

#define	xDUMP_SPECS
#define SUPPORT_FULL_PAGE
#define xSUPPRESS_DELTA_PCL
#define DIRECT_DELTA_PCL

/*
  Die folgenden Setzungen gehen direkt in die Pufferberechnung und skalieren
  im MB-Sektor, also sparsam. Die Settings sollten für A4 600x600 reichen.
*/

#define SYM_P	0x0011ul
#define SYM_H	0x0009ul
#define SYM_V0	0x0003ul
#define SYM_VR1	0x000Bul
#define SYM_VR2	0x0043ul
#define SYM_VR3	0x0083ul
#define SYM_VL1	0x000Aul
#define SYM_VL2	0x0042ul
#define SYM_VL3	0x0082ul
#define SYM_EXT	0x0081ul
#define SYM_EOL	0x1001ul
#define SYM_EOFB 0xFFFFul

#define OP_SYM_P	1
#define OP_SYM_H	2
#define OP_SYM_V0	3
#define OP_SYM_VR1	4
#define OP_SYM_VR2	5
#define OP_SYM_VR3	6
#define OP_SYM_VL1	7
#define OP_SYM_VL2	8
#define OP_SYM_VL3	9
#define OP_SYM_EXT	10
#define OP_SYM_EOL	11
#define OP_SYM_EOFB	12

typedef struct {
	int	cPixel;
	char 	*szBitMask;
	} TTemplRunSpec;

#ifdef DEFINE_VARS
#define GLOBAL
#else
#define GLOBAL extern
#endif

	/**
	Die Tabelle mit den Lauflängencodes in der Rohfassung.
	*/
extern TTemplRunSpec *arstSpecs[3];

typedef struct {
	short	        cPixel;
	unsigned short	bitMask;
	} TRunSpec;
	
/*
* Seitenmaße lt PCL-Manual :	cX=2480-2*50 (printable area)
				cY=3507-2*50
*/

#define		Y_LAST_LINE	2
#define		CRUNTABLE	120


typedef struct Tg4tInstance {
  /* code table stuff */
  int	        acRuns[2];
  TRunSpec	arsRuns[2][CRUNTABLE];
  G4T_BOOL	aabitBuffers[Y_LAST_LINE][G4T_CX_MAX+5];
  G4T_BOOL      *abitRef,*abitWork; /* only pointer */
  int	        iBuffer;
  unsigned char	*pchFullPage;
  long		lcchFullPageLine;
  long		lcFullPageLines;

  /* decoder.c */
  int		iLine,iByte;
  int		a,b;

  /* encode.c */
  unsigned char uchSymbol;
  unsigned int  cbSymbol;

  /* tiff.c */
  unsigned char *pchWrite;
  long	liWrite;
  long	liMax;

  /* map dimensions */
  int	        cxPaper,cyPaper;
  int	        xPaperOut;	/* offset of first output pixel */
  int	        cxPaperOut;	/* count of output pixels */
  int	        yPaperOut;	/* same for height */
  int	        cyPaperOut;

  /* options for application */
  G4T_BOOL	bFullPage;
  G4T_BOOL	bRotate;
  int		nFileFormat;
  G4T_BOOL	bNoLineWarnings;
  G4T_BOOL	bBacon;
  int		nClipType;
  int		nPCLResolution;
  G4T_BOOL	bVerbose;
  G4T_BOOL	bNoCheckEOL;

  /* options for g4tool */
  G4T_BOOL	bWriteOrientation; /* for PCL, mainly compatibility */
  G4T_BOOL	bDebugShowRunComp;
  G4T_BOOL	bDebugVerbose;
  G4T_BOOL	bDebugPCL;
  G4T_BOOL	bLandscape;
  G4T_BOOL	bDebugEncoder;
} Tg4tInstance;

#define THIS struct Tg4tInstance *this

/* ****************************************************************** */

#endif
