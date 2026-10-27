/*
 * state.c - definitions of the editor globals (ted.s 6057-6280, chunk 06
 * data block) with their original initial values.  Declarations and
 * comments: state.h.
 */
#include "state.h"

UWORD topskip;                  /* ted.s:6057 */
UWORD botskip;                  /* ted.s:6058 */
UWORD chcols = 128;             /* ted.s:6059 */
UWORD totsccols = 80;           /* ted.s:6060 */
UWORD edrows;                   /* ted.s:6061 */
UWORD pixrows;                  /* ted.s:6062 */
UWORD pixrows2;                 /* ted.s:6063 */
UWORD edcols;                   /* ted.s:6064 */
UWORD mousecols;                /* ted.s:6065 */
UBYTE *bitmaps[2];              /* ted.s:6066 */

UWORD cursx;                    /* ted.s:6067 */
UWORD cursy;                    /* ted.s:6068 */
LONG  scloc;                    /* ted.s:6069 */
UWORD chloc;                    /* ted.s:6070 */
UWORD cursx2;                   /* ted.s:6071 */
UWORD cursy2;                   /* ted.s:6072 */
LONG  scloc2;                   /* ted.s:6073 */
UWORD chloc2;                   /* ted.s:6074 */
WORD  cursoff;                  /* ted.s:6075 */
UWORD curstop;                  /* ted.s:6076 */
UWORD color = 1;                /* ted.s:6077 */
UWORD qualifier;                /* ted.s:6078 */
UBYTE *chmap;                   /* ted.s:6079 */
UWORD anyalt;                   /* ted.s:6080 */

struct item *firstitem;         /* ted.s:6081 */
struct item *enditem;           /* ted.s:6082 */
struct item *thisitem;          /* ted.s:6083 */
UWORD numlines;                 /* ted.s:6084 */

WORD  bloktopx, bloktopy;       /* ted.s:6085/6086 */
WORD  blokbotx, blokboty;       /* ted.s:6087/6088 */
WORD  btopx;                    /* ted.s:6089 */
UWORD btopy;                    /* ted.s:6090 */
WORD  bbotx;                    /* ted.s:6091 */
UWORD bboty;                    /* ted.s:6092 init 0 (not -1!) */
UWORD invflag;                  /* ted.s:6093 */
UWORD inverse;                  /* ted.s:6094 */

jmp_buf *memstack;              /* ted.s:6095 */
tedfh filehand;                 /* ted.s:6096 */
UWORD memalt;                   /* ted.s:6097 */

struct defblock defstart = {    /* ted.s:6099-6120 */
    8, 7, 0, 0, 1,              /* fonthite fonthite2 fontmul tokenise makeicons */
    1, 2, 3, 0,                 /* normcol labcol tokcol propcol */
    12, 6, 2,                   /* hscmarg vscmarg tabstop */
    { 8, 8, 8, 15, 15, 15, 0, 0, 0, 15, 13, 0 },   /* palit */
    0                           /* origscrows */
};

const char defname[] = "l:blitzeditor.opts";               /* ted.s:6122 */
UWORD mpalit[9] = { 15, 15, 0, 15, 0, 0, 0, 0, 0 };         /* ted.s:6125 */
UBYTE *statsloc;                                            /* ted.s:6129 */
char statstext[] = "Line:      Column:    "                 /* ted.s:6130 */
                   "Largest Mem (K):      ";
char bltext[6] = "Block";                                   /* ted.s:6133 */
char numtext[6];                                            /* ted.s:6135 */
UWORD defalt;                                               /* ted.s:6136 */
APTR req;                                                   /* ted.s:6137 */
UWORD allalt;                                               /* ted.s:6138 */
char filepath[192];                                         /* ted.s:6140 */
char filename[64];                                          /* ted.s:6142 */
const char filetext[] = "Name of file to load";             /* ted.s:6144 */
const char filetext2[] = "Name to save file as";            /* ted.s:6145 */
char blokpath[192];                                         /* ted.s:6146 */
char blokname[64];                                          /* ted.s:6148 */
const char bloktext[] = "Name of block to load";            /* ted.s:6150 */
const char bloktext2[] = "Name to save block as";           /* ted.s:6151 */
UWORD blokop;                                               /* ted.s:6152 */

UBYTE *charbuff;                /* ted.s:6157 */
UBYTE *charbuff2;               /* ted.s:6158 */
UBYTE *charbuff3;               /* ted.s:6159 */
UBYTE *charbuff4;               /* ted.s:6160 */
UBYTE *dosbuff;                 /* ted.s:6161 */

UWORD nummice;                  /* ted.s:6163 */
struct micenode *mice;          /* ted.s:6164 */
UWORD mousealt;                 /* ted.s:6165 */

const char tokeerr[] = "?????\0Token Unavailable";          /* ted.s:6167 */

const char error1[] = "Couldn't open file!";                /* ted.s:6260 */
const char error2[] = "Error reading file!";                /* ted.s:6262 */
const char error3[] = "Error writing file!";                /* ted.s:6264 */

WORD returncode;                /* ted.s:6267 */
APTR external;                  /* ted.s:6268 */
struct tokennode *tokens;       /* ted.s:6269 */
UBYTE *menitems;                /* ted.s:6270 */
APTR loadxtra;                  /* ted.s:6272 */
APTR savextra;                  /* ted.s:6273 */
APTR menurout;                  /* ted.s:6275 */
APTR cleanup;                   /* ted.s:6276 */
UBYTE colon = 255;              /* ted.s:6277 */
UBYTE quote = 255;              /* ted.s:6278 */
UBYTE label = 255;              /* ted.s:6279 */
UBYTE remark = 255;             /* ted.s:6280 */

UBYTE findreqla16[64];          /* ted.s:7061 (chunk 07 data) */
UBYTE findreqla17[64];          /* ted.s:7072 (chunk 07 data) */

BOOL edquit;                    /* new: escape -> leave mainedit */
LONG ev_d7;                     /* new: d7 of the dispatch */
UWORD ev_d6;                    /* new: d6 of the dispatch */
