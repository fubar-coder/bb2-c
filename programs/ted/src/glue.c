/*
 * glue.c - register conventions between TED and blitz2 (see glue.h):
 * callregs, the comdata entry stubs (REGSTUB) with their C adaptors, the
 * callbacks into blitz2, the VBlank server stub.  Plus the plain C file/
 * memory hooks of hooks.h for the Amiga (dos/exec 1:1).
 *
 * This file and glue_start.c are the only places with assembler (68000,
 * MIT syntax, gcc symbols with '_').
 */
#include <proto/exec.h>
#include <proto/dos.h>
#include "ted.h"

/* ------------------------------------------------------------ callregs */

/* UWORD callregs(APTR fn, struct m68kregs *r) - from blitz2-c/src/glue.c:
   loads d0-d7/a0-a6 from *r, calls fn, stores all registers back and
   returns the CCR (captured with Scc: `move ccr` does not exist on the
   68000, `move sr` is privileged on 68010+). */
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _callregs\n"
"_callregs:\n"
"   movem.l d2-d7/a2-a6,-(sp)\n"    /* 44 bytes; fn at 48(sp), r at 52(sp) */
"   move.l  52(sp),a1\n"
"   move.l  a1,-(sp)\n"             /* [r] */
"   pea     1f\n"                   /* [ret][r] */
"   move.l  56(sp),-(sp)\n"         /* [fn][ret][r] */
"   movem.l (a1),d0-d7/a0-a6\n"
"   rts\n"                          /* 'call' fn, it returns to 1: */
"1: scs     -(sp)\n"
"   svs     -(sp)\n"
"   seq     -(sp)\n"
"   smi     -(sp)\n"                /* [N][Z][V][C][r] */
"   move.l  a6,-(sp)\n"
"   move.l  12(sp),a6\n"            /* a6 = r */
"   movem.l d0-d7/a0-a5,(a6)\n"
"   move.l  (sp)+,56(a6)\n"         /* r->a[6] */
"   moveq   #0,d0\n"
"   tst.b   (sp)+\n"
"   beq.s   2f\n"
"   or.b    #8,d0\n"
"2: tst.b   (sp)+\n"
"   beq.s   3f\n"
"   or.b    #4,d0\n"
"3: tst.b   (sp)+\n"
"   beq.s   4f\n"
"   or.b    #2,d0\n"
"4: tst.b   (sp)+\n"
"   beq.s   5f\n"
"   or.b    #1,d0\n"
"5: addq.l  #4,sp\n"
"   movem.l (sp)+,d2-d7/a2-a6\n"
"   rts\n"
);

/* ------------------------------------------------------------- DIVU.W */

/* ULONG divu_w(ULONG d0, UWORD src) (declared in core/fileio.h and glue.h):
   the 68000 instruction `divu.w src,d0` itself.  Result (remainder << 16)
   | quotient; on overflow (quotient > $FFFF) the 68000 sets V and leaves
   d0 unchanged; src = 0 raises the zero divide trap (exception vector 5 ->
   the task's tc_TrapCode, normally the "division by zero" guru) exactly
   like the DIVU instructions of ted.s.  Used for every divu of ted.s and
   for the file requester (filereq.c: fq_divu_w).  C's `/` and `%` on 32
   bit values would call libnix's __udivsi3/__umodsi3, which need
   UtilityBase - never opened (no libnix startup).  ted.s has no DIVS,
   MULS or 32 bit multiplication, so no signed variant is needed (the
   build checks that no arithmetic helper is linked: ctest nohelpers). */
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _divu_w\n"
"_divu_w:\n"
"   move.l  4(sp),d0\n"
"   divu.w  10(sp),d0\n"            /* UWORD argument: low word of its long slot */
"   rts\n"
);

/* -------------------------------------------- outmem, memstack == 0 */

/* NORETURN void memcont_nostack(void) - see glue.h.  BUG(orig) (#370) (O1,
   survey-03 I.6): before the first dispatch of mainedit memstack is 0;
   the asm continues memcont with a7 = 0.  Reproduced instruction by
   instruction; only the called routines are the C versions (their own
   stack use differs from the asm, so the exact crash point may too). */
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _memcont_nostack\n"
"_memcont_nostack:\n"
GLUE_LOAD_A4
"   suba.l  a7,a7\n"                /* move.l memstack(pc),a7 (= 0) */
"   move.w  " GLUE_VAR("memalt") ",d0\n"   /* memcont */
"   beq.s   1f\n"
"   jsr     _printmem\n"
"1: move.w  " GLUE_VAR("mousealt") ",d0\n"
"   beq.s   2f\n"
"   jsr     _showmice\n"
"2: move.b  (a7)+,d0\n"            /* the flag byte: address 0 (a7 += 2) */
"   tst.b   d0\n"
"   beq.s   3f\n"
"   jsr     _showvprop\n"
"3: jmp     _mainedit\n"            /* bra mainedit */
);

/* --------------------------------------------------- comdata entries */

/* REGSTUB(name): `name_stub` saves d0-d7/a0-a6, loads TED's a4, calls
   name_c(frame), restores the (modified) frame, `tst.l d0`, rts. */
#define REGSTUB(name)                                   \
__asm__(                                                \
"   .text\n"                                            \
"   .even\n"                                            \
"   .globl  _" #name "_stub\n"                          \
"_" #name "_stub:\n"                                    \
"   movem.l d0-d7/a0-a6,-(sp)\n"                        \
GLUE_LOAD_A4                                            \
"   move.l  sp,-(sp)\n"                                 \
"   jsr     _" #name "_c\n"                             \
"   addq.l  #4,sp\n"                                    \
"   movem.l (sp)+,d0-d7/a0-a6\n"                        \
"   tst.l   d0\n"                                       \
"   rts\n"                                              \
)

/* REGSTUB_CCR(name): like REGSTUB, but name_c returns the CCR the entry
   returns with (the asm routine's flags are not those of `tst.l d0`).
   `move to ccr` exists on the 68000; movem/lea leave the flags alone. */
#define REGSTUB_CCR(name)                               \
__asm__(                                                \
"   .text\n"                                            \
"   .even\n"                                            \
"   .globl  _" #name "_stub\n"                          \
"_" #name "_stub:\n"                                    \
"   movem.l d0-d7/a0-a6,-(sp)\n"                        \
GLUE_LOAD_A4                                            \
"   move.l  sp,-(sp)\n"                                 \
"   jsr     _" #name "_c\n"                             \
"   addq.l  #4,sp\n"                                    \
"   move.w  d0,-(sp)\n"                                 \
"   movem.l 2(sp),d0-d7/a0-a6\n"                        \
"   move.w  (sp),ccr\n"                                 \
"   lea     62(sp),sp\n"                                \
"   rts\n"                                              \
)

#define SETW(r, v) ((r) = ((r) & 0xffff0000UL) | (UWORD)(v))   /* op.w */

/* The adaptors: registers in, C routine, results out (survey-06 C table).
   Not static: called from the asm stubs. */
void screenclose_c(struct m68kregs *r);
void screenopen2_c(struct m68kregs *r);
void openreq_c(struct m68kregs *r);
void closereq_c(struct m68kregs *r);
void getinput_c(struct m68kregs *r);
void error_c(struct m68kregs *r);
void detokallz_c(struct m68kregs *r);
void tokall_c(struct m68kregs *r);
void ezfreq_c(struct m68kregs *r);
void refgads_c(struct m68kregs *r);
void getinput2_c(struct m68kregs *r);
void colors_c(struct m68kregs *r);
void halfcolors_c(struct m68kregs *r);
void tokone_c(struct m68kregs *r);
UWORD openio_c(struct m68kregs *r);
void closeio_c(struct m68kregs *r);
void actgad_c(struct m68kregs *r);
void pointeron_c(struct m68kregs *r);
void sleepon_c(struct m68kregs *r);
void extalloc_c(struct m68kregs *r);

void screenclose_c(struct m68kregs *r) { (void)r; screenclose(); }        /* -8 */
void screenopen2_c(struct m68kregs *r) { (void)r; screenopen2(); }        /* -4 */
void openreq_c(struct m68kregs *r)                                        /* 0 */
{
    openreq((struct Requester *)r->a[0]);
}
void closereq_c(struct m68kregs *r) { (void)r; closereq(); }              /* 4 */
void getinput_c(struct m68kregs *r)                                       /* 8 */
{
    UWORD d6 = r->d[6];

    r->d[7] = getinput(&d6);
    SETW(r->d[6], d6);
    r->d[0] = (ULONG)-1;        /* asm: d0 = -1, Z=0 */
}
void error_c(struct m68kregs *r) { error((const char *)r->a[0]); }        /* 12 */
void detokallz_c(struct m68kregs *r) { (void)r; detokallz(); }            /* 16 */
void tokall_c(struct m68kregs *r) { (void)r; tokall(); }                  /* 20 */
void ezfreq_c(struct m68kregs *r)                                         /* 24 */
{
    r->d[0] = (ULONG)ezfreq((char *)r->a[0], (char *)r->a[1], (const char *)r->a[2]);
    r->a[3] = (ULONG)&reqstruct[3]; /* the asm leaves a3 = reqstruct+12 (lea + 3 x (a3)+;
                                   filerequest+36 saves/restores d1-a6) */
}
void refgads_c(struct m68kregs *r)                                        /* 28 */
{
    refgads((struct Gadget *)r->a[0], (LONG)r->d[0]);
}
void getinput2_c(struct m68kregs *r)                                      /* 32 */
{
    UWORD d6 = r->d[6];

    r->d[7] = getinput2(&d6);
    SETW(r->d[6], d6);
    r->d[0] = r->d[7] ? (ULONG)-1 : 0;  /* asm: Z=1 if nothing */
}
void colors_c(struct m68kregs *r) { (void)r; colors(); }                  /* 48 */
void halfcolors_c(struct m68kregs *r) { (void)r; halfcolors(); }          /* 52 */
void tokone_c(struct m68kregs *r)                                         /* 56 */
{
    SETW(r->d[5], tokone((UBYTE *)r->a[0]));
}
UWORD openio_c(struct m68kregs *r)                                        /* 60 */
{
    UWORD ccr;

    r->d[0] = openio(&ccr);     /* d0 and the flags as the asm leaves them */
    return ccr;
}
void closeio_c(struct m68kregs *r) { (void)r; closeio(); }                /* 64 */
void actgad_c(struct m68kregs *r)                                         /* 68 */
{
    r->d[0] = actgad((struct Gadget *)r->a[0]);
}
void pointeron_c(struct m68kregs *r) { (void)r; pointeron(); }            /* 72 */
void sleepon_c(struct m68kregs *r) { (void)r; sleepon(); }                /* 76 */
void extalloc_c(struct m68kregs *r)                                       /* 88 */
{
    r->d[0] = (ULONG)extalloc(r->d[0], r->d[1]);
}

REGSTUB(screenclose);
REGSTUB(screenopen2);
REGSTUB(openreq);
REGSTUB(closereq);
REGSTUB(getinput);
REGSTUB(error);
REGSTUB(detokallz);
REGSTUB(tokall);
REGSTUB(ezfreq);
REGSTUB(refgads);
REGSTUB(getinput2);
REGSTUB(colors);
REGSTUB(halfcolors);
REGSTUB(tokone);
REGSTUB_CCR(openio);
REGSTUB(closeio);
REGSTUB(actgad);
REGSTUB(pointeron);
REGSTUB(sleepon);
REGSTUB(extalloc);

/* ------------------------------------------------ callbacks into blitz2 */

/* ted.s:1555 extmenu: jsr (a0) with d6.w item, a5 firstitem, d7.w numlines;
   the upper words of d6/d7 are undefined in the asm (0 here); a4 =
   thisitem as in the handler.  Returns d0.w. */
WORD call_menurout(UWORD d6)
{
    struct m68kregs r = {{0}};

    r.d[6] = d6;
    r.d[7] = numlines;
    r.a[5] = (ULONG)firstitem;
    r.a[4] = (ULONG)thisitem;
    callregs(menurout, &r);
    return (WORD)r.d[0];
}

/* ted.s:2771 (doclean): jsr (cleanup) - all registers saved by the asm
   (callregs saves TED's) */
void call_cleanup(void)
{
    struct m68kregs r = {{0}};

    callregs(cleanup, &r);
}

/* fileopen2 ted.s:710 / realsave ted.s:795: jsr (a5) with d7 = handle,
   a6 = DOSBase (the caller closes d7 afterwards with the a6 the callback
   left - the C caller uses DOSBase, same library) */
static void call_xtra(APTR fn, tedfh fh)
{
    struct m68kregs r = {{0}};

    r.d[7] = fh;
    r.a[5] = (ULONG)fn;
    r.a[6] = (ULONG)DOSBase;
    callregs(fn, &r);
}

void call_loadxtra(tedfh fh) { call_xtra(loadxtra, fh); }
void call_savextra(tedfh fh) { call_xtra(savextra, fh); }

/* ------------------------------------------------------ VBlank server */

/* ted.s:306 myint.is_Code: a1 = is_Data, a5 = code; d0/d1/a0/a1/a5/a6 are
   scratch for a server, a4 is not -> saved.  Returns d0 = 0, Z=1 (ted.s:302
   `moveq #0,d0`): the server chain continues. */
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _myint_stub\n"
"_myint_stub:\n"
"   move.l  a4,-(sp)\n"
GLUE_LOAD_A4
"   jsr     _myroutine\n"
"   move.l  (sp)+,a4\n"
"   moveq   #0,d0\n"
"   rts\n"
);

/* ------------------------------------------- file / memory hooks (C) */

APTR ted_alloc(ULONG size, ULONG flags) { return AllocMem(size, flags); }
void ted_free(APTR p, ULONG size) { FreeMem(p, size); }
tedfh ted_open(const char *name, LONG mode) { return Open((CONST_STRPTR)name, mode); }
LONG ted_read(tedfh fh, APTR buf, LONG len) { return Read(fh, buf, len); }
LONG ted_write(tedfh fh, const void *buf, LONG len) { return Write(fh, (APTR)buf, len); }
void ted_close(tedfh fh) { Close(fh); }
LONG ted_rename(const char *o, const char *n) { return Rename((CONST_STRPTR)o, (CONST_STRPTR)n); }
LONG ted_delete(const char *name) { return DeleteFile((CONST_STRPTR)name); }
tedfh ted_lock(const char *name, LONG mode) { return Lock((CONST_STRPTR)name, mode); }
void ted_unlock(tedfh lock) { UnLock(lock); }
