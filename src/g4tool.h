#ifndef H_G4TOOL
#define H_G4TOOL

typedef int     G4T_BOOL;
typedef int     G4T_RC;

#define		G4T_CX_MAX	5000
#define		G4T_CY_MAX	8000

#define         G4T_EOK         0
#define         G4T_EGENERAL    1


/* ====================== function prototypes =========================== */

/* PBM */

struct Tg4tInstance *g4tInit(void);
void g4tExit(struct Tg4tInstance *this);

G4T_BOOL g4tWriteHeader(struct Tg4tInstance *this,FILE *f);
G4T_BOOL g4tFlushLine(struct Tg4tInstance *this,FILE *f);
G4T_BOOL g4tClosePage(struct Tg4tInstance *this,FILE *f);
void g4tCenterPage(struct Tg4tInstance *this);
void g4tEncodePageLine(struct Tg4tInstance *this);
G4T_RC  g4tRotatePage(struct Tg4tInstance *this);

/* internal */ G4T_BOOL g4tFlushLinePBM(struct Tg4tInstance *this,FILE *f);

/* PCL */

/* internal */ G4T_BOOL g4tWriteHeadPCL(struct Tg4tInstance *this,FILE *f);
/* internal */ G4T_BOOL g4tDumpPagePCL(struct Tg4tInstance *this,FILE *f);
/* internal */ G4T_BOOL g4tFlushLinePCL(struct Tg4tInstance *this,FILE *f);

/* ENCODE */

G4T_RC   g4tEncodePage(struct Tg4tInstance *this, FILE *f);
/* internal */ G4T_BOOL g4tWriteHeadG4(struct Tg4tInstance *this,FILE *f, G4T_BOOL bTiff);
/* internal */ G4T_BOOL g4tClosePageG4(struct Tg4tInstance *this,FILE *f, G4T_BOOL bTiff);
/* internal */ G4T_BOOL g4tFlushLineG4(struct Tg4tInstance *this,FILE *f);

/* DECODE */

G4T_RC g4tDecodePage(struct Tg4tInstance *this, FILE *f);
/* internal */ G4T_BOOL g4tSetTables(struct Tg4tInstance *this);

/* TIFF */

G4T_BOOL g4tTiffEnter(struct Tg4tInstance *this,unsigned char uch);
G4T_BOOL g4tTiffClose(struct Tg4tInstance *this,FILE *f, int cx, int cy, int idCompression);
G4T_BOOL g4tTiffCreate(struct Tg4tInstance *this,long lcchEstimated);
/* $Extended$File$Info$
 */


#endif
