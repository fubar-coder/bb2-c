/*
 * rinonvolatilelib - RWE userlib 35 (Reflective Images, 1994): the
 * nonvolatile.library of OS 3.1 - IsNonvolatile, StoreNVData, GetCopyNV,
 * FreeNVData, GetNVList.
 *
 * C translation of rwelibsrc/nonvolatile.bb; reference BSS2
 * RINONVOLATILELIB.OBJ (the reference build of the source equals it byte
 * for byte, docs/rwelibs/README.md 3.1).  Notes:
 * docs/rwelibs/port/rinonvolatilelib.md.
 *
 * init opens nonvolatile.library (any version), finit closes it.  Every
 * call returns 0 when the library is not open; otherwise the arguments go
 * to the system call as they are, killRequesters = -1 (StoreNV, GetCopyNV,
 * GetNVList) - DeleteNV of StoreNVData with 0.  StoreNVData and GetCopyNV
 * are Amiga mode only.  StoreNVData deletes the item first and gives
 * StoreNV the length in tens of bytes, rounded up.
 *
 * No assembler besides the thunks.
 *
 * BUG(orig):
 *   - StoreNVData: DIVU.w #10 overflows for lengths of 655360 bytes and
 *     more (the quotient does not fit into a word: d3 stays unchanged),
 *     StoreNV then gets the low word of the length (+1 if the high word
 *     is not 0) as the number of tens of bytes (#764).
 *   - StoreNVData: the DeleteNV before StoreNV is unconditional (an item
 *     that cannot be stored again is lost), and its result is ignored
 *     (#763).
 *   - StoreNVData returns StoreNV's result (!acommand #word): 0 means
 *     stored, otherwise an NVERR_... code - and the "library not open"
 *     case returns 0 as well, "stored" (#765).
 *
 * finit closes the library but leaves libbase set: no defect, it is the
 * last call.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#define __NOLIBBASE__
#define NONVOLATILE_BASE_NAME libbase
#include <proto/nonvolatile.h>
#undef __NOLIBBASE__

/* entry points (thunks, below) */
void _isnonvolatile(void), _storenvdata(void), _getcopynv(void);
void _freenvdata(void), _getnvlist(void), _libinit(void), _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#rinonvolatilelib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_rinonvolatilelib, init, BL_NORET, finit, 0);

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_isnonvolatile, 0, 0);
BLITZ_NAME("IsNonvolatile", "-- returns true if library is open");

BLITZ_ACOMMAND(BL_WORD);
BLITZ_ARGS(BL_UNKNOWN, BL_UNKNOWN, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_storenvdata), 0, 0);
BLITZ_NAME("StoreNVData", "AppName,ItemName,data,byte_length");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_UNKNOWN, BL_UNKNOWN);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_getcopynv), 0, 0);
BLITZ_NAME("GetCopyNV", "AppName,ItemName");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_freenvdata, 0, 0);
BLITZ_NAME("FreeNVData", "APTR");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_UNKNOWN);
BLITZ_LIBS0();
BLITZ_SUBS(_getnvlist, 0, 0);
BLITZ_NAME("GetNVList", "appname");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

static struct Library *libbase;                         /* libbase: Dc.l 0 */
static const char libname[] = "nonvolatile.library";

/* init: OpenLibrary(libname, 0), the base also in d0 */
static ULONG BLITZ_ROUTINE libinit_c(void)
{
    libbase = OpenLibrary((CONST_STRPTR)libname, 0);
    return (ULONG)libbase;
}

/* finit: CloseLibrary if open (libbase stays set, no defect, see above) */
static void BLITZ_ROUTINE libfinit_c(void)
{
    if (libbase)
        CloseLibrary(libbase);
}

/* IsNonvolatile: SNE d0:EXT.w d0 - -1/0 in the low word */
static ULONG BLITZ_ROUTINE isnonvolatile_c(void)
{
    return libbase ? 0xffff : 0;
}

/* StoreNVData app,item,data,length (d0-d3) */
static ULONG BLITZ_ROUTINE storenvdata_c(CONST_STRPTR app BL_REG(d0),
                                         CONST_STRPTR item BL_REG(d1),
                                         CONST_APTR data BL_REG(d2),
                                         ULONG len BL_REG(d3))
{
    ULONG q, tens;

    if (!libbase)
        return 0;                                       /* ReturnFalse */
    DeleteNV(app, item, 0);
    q = bl_divu_w(len, 10);                     /* DIVU.w #10,d3, BUG(orig) #764 */
    tens = (UWORD)q;
    if (q >> 16)                                        /* remainder */
        tens++;
    return StoreNV(app, item, data, tens, -1);
}

/* GetCopyNV(app,item) (d0/d1) */
static ULONG BLITZ_ROUTINE getcopynv_c(CONST_STRPTR app BL_REG(d0),
                                       CONST_STRPTR item BL_REG(d1))
{
    if (!libbase)
        return 0;
    return (ULONG)GetCopyNV(app, item, -1);
}

/* FreeNVData data (d0) */
static ULONG BLITZ_ROUTINE freenvdata_c(APTR data BL_REG(d0))
{
    if (!libbase)
        return 0;
    FreeNVData(data);
    return (ULONG)data;
}

/* GetNVList(app) (d0) */
static ULONG BLITZ_ROUTINE getnvlist_c(CONST_STRPTR app BL_REG(d0))
{
    if (!libbase)
        return 0;
    return (ULONG)GetNVList(app, -1);
}

/* Registers: the original changes d0 and, through the system calls,
   d1/a0/a1 (and a6, StoreNVData also a2); the thunks keep d1/a0/a1, the
   C code a2/a6.  IsNonvolatile keeps the upper word of d0 (EXT.w). */
BLITZ_THUNK(_libinit, libinit_c, "d1/a0-a1");
BLITZ_THUNK(_libfinit, libfinit_c, "d0-d1/a0-a1");
BLITZ_THUNK_POST(_isnonvolatile, isnonvolatile_c, "d0-d1/a0-a1", "move.w d0,2(sp)");
BLITZ_THUNK(_storenvdata, storenvdata_c, "d1/a0-a1");
BLITZ_THUNK(_getcopynv, getcopynv_c, "d1/a0-a1");
BLITZ_THUNK(_freenvdata, freenvdata_c, "d1/a0-a1");
BLITZ_THUNK(_getnvlist, getnvlist_c, "d1/a0-a1");
