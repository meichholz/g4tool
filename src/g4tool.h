#ifndef H_G4TOOL
#define H_G4TOOL

#define G4T_BOOL int

/* ====================== function prototypes =========================== */

/** MAIN */

struct Tg4tInstance *g4tInit(void);
void g4tExit(struct Tg4tInstance *this);

void g4tCenterPage(struct Tg4tInstance *this);
BOOL g4tWriteHeader(struct Tg4tInstance *this,FILE *f);
BOOL g4tFlushLine(struct Tg4tInstance *this,FILE *f);
BOOL g4tClosePage(struct Tg4tInstance *this,FILE *f);
void g4tEncodePageLine(struct Tg4tInstance *this);

/* PCL */

BOOL g4tWriteHeadPCL(struct Tg4tInstance *this,FILE *f);
BOOL g4tDumpPagePCL(struct Tg4tInstance *this,FILE *f);
BOOL g4tFlushLinePCL(struct Tg4tInstance *this,FILE *f);

/* ENCODE */

BOOL g4tWriteHeadG4(struct Tg4tInstance *this,FILE *f, BOOL bTiff);
BOOL g4tClosePageG4(struct Tg4tInstance *this,FILE *f, BOOL bTiff);
BOOL g4tEncodePage(struct Tg4tInstance *this);
BOOL g4tFlushLineG4(struct Tg4tInstance *this,FILE *f);

/* DECODE */

BOOL g4tDecodePage(struct Tg4tInstance *this);
BOOL g4tSetTables(struct Tg4tInstance *this);

/* TIFF */

BOOL g4tTiffEnter(struct Tg4tInstance *this,unsigned char uch);
BOOL g4tTiffClose(struct Tg4tInstance *this,FILE *f, int cx, int cy, int idCompression);
BOOL g4tTiffCreate(struct Tg4tInstance *this,long lcchEstimated);
/* $Extended$File$Info$
 */


#endif
