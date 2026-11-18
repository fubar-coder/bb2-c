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
 * Fixed (intended deviations):
 *   - StoreNVData's DIVU.w #10 overflowed for lengths of 655360 bytes and
 *     more (the quotient does not fit into a word: d3 stays unchanged),
 *     StoreNV then got the low word of the length (+1 if the high word
 *     is not 0) as the number of tens of bytes; the length is now
 *     divided as a long (#764).
 *   - StoreNVData returned StoreNV's result (!acommand #word): 0 when
 *     stored, otherwise an NVERR_... code, the reverse of the documented
 *     TRUE/FALSE; it now returns -1 when stored, 0 when not (#1085).
 * No defects:
 *   - StoreNVData returns 0 when the library is not open (the common
 *     ReturnFalse), which is FALSE, not stored, as documented (#765).
 *   - StoreNVData deletes the item before StoreNV, always, and ignores
 *     DeleteNV's result: intended, it makes room for the new version in
 *     a small NVRAM (StoreNV alone would need room for both); an item
 *     that cannot be stored again is lost then (#763).
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
void _freenvdata(void), _getnvlist(void), rinonvolatilelib_libinit(void);
void rinonvolatilelib_libfinit(void);

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

BLITZ_NULLSUB_AT(finit, rinonvolatilelib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, rinonvolatilelib_libinit, 0, 0);

BLITZ_LIBFIN();

static struct Library *libbase;                         /* libbase: Dc.l 0 */
static const char libname[] = "nonvolatile.library";

/**
 * @brief init: open nonvolatile.library (any version).
 * @return d0: the library base, 0 if it could not be opened (BL_NORET:
 *         not used by the program)
 */
static ULONG BLITZ_ROUTINE libinit_c(void)
{
    libbase = OpenLibrary((CONST_STRPTR)libname, 0);
    return (ULONG)libbase;
}

/**
 * @brief finit: close nonvolatile.library if it is open.
 *
 * libbase stays set (no defect: it is the last call, see the head).
 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    if (libbase)
        CloseLibrary(libbase);
}

/**
 * @brief IsNonvolatile: true if nonvolatile.library is open.
 *
 * SNE d0:EXT.w d0 - a Blitz truth value as a word (-1/0 in the low word,
 * the thunk keeps the upper word of d0); not BB_BOOL, which is a long.
 * @return d0.w: -1 ($FFFF) if open, 0 if not
 */
static ULONG BLITZ_ROUTINE isnonvolatile_c(void)
{
    return libbase ? 0xffff : 0;
}

/**
 * @brief StoreNVData AppName,ItemName,data,byte_length: store data in
 * nonvolatile memory (Amiga mode only).
 *
 * Deletes the item first (DeleteNV with killRequesters 0, always, its
 * result ignored: room for the new version in a small NVRAM, no defect;
 * an item that cannot be stored again is lost, #763), then StoreNV with the length in tens of bytes, rounded up,
 * and killRequesters -1.  The length is divided as a long (fixed, #764:
 * the original's DIVU.w #10 overflows for 655360 bytes and more, StoreNV
 * then gets the low word of the length, +1 if the high word is not 0).
 * @param app   d0: the application name
 * @param item  d1: the item name
 * @param data  d2: the data
 * @param len   d3: its length in bytes
 * @return d0.w: -1 (TRUE) when StoreNV stored the data (its result 0),
 *         0 (FALSE) when it did not (an NVERR_... code) and when the
 *         library is not open (fixed, #1085: the original returned
 *         StoreNV's result, 0 = stored; no defect then in the 0 without
 *         the library, #765)
 */
static ULONG BLITZ_ROUTINE storenvdata_c(CONST_STRPTR app BL_REG(d0),
                                         CONST_STRPTR item BL_REG(d1),
                                         CONST_APTR data BL_REG(d2),
                                         ULONG len BL_REG(d3))
{
    ULONG hi, lo, tens;

    if (!libbase)
        return BB_FALSE;                                /* ReturnFalse */
    DeleteNV(app, item, 0);
    /* len / 10 as a long with two DIVU.w (fixed, #764): the high word's
       remainder (< 10) goes in front of the low word, so the second
       quotient fits into a word */
    hi = bl_divu_w(len >> 16, 10);
    lo = bl_divu_w((hi & 0xffff0000) | (len & 0xffff), 10);
    tens = (hi << 16) | (UWORD)lo;
    if (lo >> 16)                                       /* remainder */
        tens++;
    /* Fixed (#1085): TRUE/FALSE as documented, not StoreNV's UWORD */
    return (UWORD)StoreNV(app, item, data, tens, -1) == 0 ? BB_TRUE : BB_FALSE;
}

/**
 * @brief GetCopyNV(AppName,ItemName): a copy of an item of nonvolatile
 * memory (Amiga mode only).
 *
 * GetCopyNV with killRequesters -1.
 * @param app   d0: the application name
 * @param item  d1: the item name
 * @return d0: the copy (free it with FreeNVData), 0 if there is none or
 *         the library is not open
 */
static ULONG BLITZ_ROUTINE getcopynv_c(CONST_STRPTR app BL_REG(d0),
                                       CONST_STRPTR item BL_REG(d1))
{
    if (!libbase)
        return 0;
    return (ULONG)GetCopyNV(app, item, -1);
}

/**
 * @brief FreeNVData APTR: free the data of GetCopyNV or GetNVList.
 * @param data  d0: the data
 * @return d0: the data (0 if the library is not open); a statement, the
 *         program does not see it
 */
static ULONG BLITZ_ROUTINE freenvdata_c(APTR data BL_REG(d0))
{
    if (!libbase)
        return 0;
    FreeNVData(data);
    return (ULONG)data;
}

/**
 * @brief GetNVList(appname): the list of the items of an application in
 * nonvolatile memory.
 *
 * GetNVList with killRequesters -1.
 * @param app  d0: the application name
 * @return d0: the list (a struct MinList, free it with FreeNVData), 0 if
 *         the library is not open
 */
static ULONG BLITZ_ROUTINE getnvlist_c(CONST_STRPTR app BL_REG(d0))
{
    if (!libbase)
        return 0;
    return (ULONG)GetNVList(app, -1);
}

/* Registers: the original changes d0 and, through the system calls,
   d1/a0/a1 (and a6, StoreNVData also a2); the thunks keep d1/a0/a1, the
   C code a2/a6.  IsNonvolatile keeps the upper word of d0 (EXT.w). */
BLITZ_THUNK(rinonvolatilelib_libinit, libinit_c, "d1/a0-a1");
BLITZ_THUNK(rinonvolatilelib_libfinit, libfinit_c, "d0-d1/a0-a1");
BLITZ_THUNK_POST(_isnonvolatile, isnonvolatile_c, "d0-d1/a0-a1", "move.w d0,2(sp)");
BLITZ_THUNK(_storenvdata, storenvdata_c, "d1/a0-a1");
BLITZ_THUNK(_getcopynv, getcopynv_c, "d1/a0-a1");
BLITZ_THUNK(_freenvdata, freenvdata_c, "d1/a0-a1");
BLITZ_THUNK(_getnvlist, getnvlist_c, "d1/a0-a1");
