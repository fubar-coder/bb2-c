/*
 * hostenv.h - the host test environment of the TED core: hook stubs
 * (stubs.c), token list (toklist.c), the oracle data (oracle.hpp).
 *
 * The stubs implement every function of src/core/hooks.h:
 *   - memory: malloc with a header (requested size + slack for the larger
 *     host struct item); ted_free with a different size counts a hazard
 *   - files: stdio, handles are indexes into a table (0 = failure)
 *   - UI hooks: counted in host_calls[] (and logged in order if
 *     host_trace is set); the ones with state effects do them (scroll
 *     hooks -> chmap_* of the core, setdefs copies the block)
 *   - outmem: longjmp to *host_outmem (abort if NULL)
 *   - ORIG_HAZARD: counted per message (host_hazards)
 */
#ifndef TED_HOSTENV_H
#define TED_HOSTENV_H

#include <setjmp.h>
#include "tedcore.h"
#include "hooks.h"
#include "state.h"

/* set up the globals like outside does for blitz2: buffers (one block,
   dosbuff + charbuff1..3 as in ted.s:41), chmap (rows x chcols), token
   list from tokfile (tests/data/ted-tokens.txt), colon/quote/label/remark =
   ':' '"' '.' ';', tokenise = 1.  tokfile may be NULL (no token list,
   tokenise = 0). */
void hostenv_init(const char *tokfile, UWORD rows);
/* free the line list (if any) without the core: for resetting between
   files independent of freeall */
void hostenv_droplist(void);

/* token list from a file "XXXX name" per line (toklist.c); NULL if missing */
struct tokennode *host_loadtokens(const char *path);
/* the token list of tedsrc tok/detok: the list file LIST (as above) when
   BLITZ2 is NULL, else made from the compiler BLITZ2 and the NLIBS library
   or BigLib files LIBS (tokref.c).  NULL after a message on stderr with
   prog. */
struct tokennode *host_tokens(const char *prog, const char *list, const char *blitz2,
                              const char *const *libs, int nlibs);
/* read a whole file (malloc'd, *len); NULL if missing */
UBYTE *host_readfile(const char *path, long *len);

/* hook bookkeeping */
enum hostcall {
    HC_ERROR, HC_GETBNAME, HC_POINTERON, HC_SLEEPON, HC_SHOWFNAME, HC_BEEPEM,
    HC_SHOWALL, HC_SHOWTOPITEM, HC_SHOWBOTITEM, HC_SHOWTHISITEM,
    HC_BLOKPRNT2, HC_BLOKMENUS, HC_SCROLLUP, HC_SCRUPALL, HC_SCROLLDOWN,
    HC_SCRDOWNALL, HC_CLS, HC_SCROLLRITE, HC_SCROLLLEFT, HC_REFRESH,
    HC_SHOWHPROP, HC_SHOWVPROP, HC_SETDEFS, HC_DOCLEAN, HC_LOADXTRA,
    HC_SAVEXTRA, HC_OUTMEM, HC_COUNT
};
extern int host_calls[HC_COUNT];
extern int host_trace;                  /* print every hook call */
extern jmp_buf *host_outmem;            /* outmem target */
extern int host_hazards;                /* ORIG_HAZARD count */
extern const char *host_lasterror;      /* last error() text */
extern BOOL host_icongad, host_findcase;
extern UBYTE *host_bname;               /* getbname result */
void host_resetcalls(void);

#endif
