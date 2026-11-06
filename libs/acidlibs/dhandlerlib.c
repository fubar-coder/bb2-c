/*
 * dhandlerlib - command library 1 (no tokens): connects the debugger
 * runtime (debuglib 64060) with the debugger's user interface when a
 * program is RUN from the editor (blitz2.s dopass: uselib(dhandlerlib)
 * together with debuglib; executables get rundebuglib instead).
 *
 * Translation of bblibsrc/dhandlerlib ("v1.1 - added popinput and
 * popoutput!"); the CD's binary matches the source.  `#dhandlerlib` is not
 * defined in libnums.bb, the binary has 1 (docs/libs/cmdlibs-decl.md):
 * LIB_dhandlerlib in libnums.h.
 *
 * init (codeinit) gets d0 = the BPTR of the debugger program loaded by
 * the editor (blitz2.s makeinits: move.l #dseg,d0, `defaultdbug`;
 * bblibsrc/dhandler.bb2 is such a program), a6 = dos, d6 = graphics.  It
 * starts the debugger as the process "Blitz ][ Debug Proc": the segment
 * is a copy of the code template mystart..mystart_f (memlib globalloc),
 * patched with the address of ltable (operand of off_1) and of progadd
 * (operand of off_2).  The process puts ltable into its tc_UserData (the
 * debugger reads it from there: d_table), calls the debugger's code
 * (seglist*4+4) and sets taskdone when that returns.  ltable is the
 * debugger's way into debuglib: l0..l8 call debuglib's tokens $D500..
 * $D508; l2 (dosethandler) also remembers the debugger's handler table.
 * init waits (WaitTOF) until the debugger has called l2, then installs
 * check_c as vertical blank server (intlib SetInt, $8035: level 5):
 * Ctrl-LAlt-C stops the program (debuglib dostop), once per key press
 * (laststop = the keys were up since the last stop; Z set on return, the
 * server chain goes on).
 * finit (codefinit) removes the server, calls the debugger's cleanup
 * handler (handler table +20: the debugger ends), waits until the process
 * has returned (taskdone) and frees the segment.
 *
 * C: the header and the token table (the libmacs macros).
 * Assembler: everything else, kept as in the original, because
 *   - mystart..mystart_f is a code template that is copied and patched
 *     at off_1/off_2 (the immediate/absolute operands), so its layout and
 *     encodings are fixed;
 *   - init and finit only call other libraries, the OS and the debugger
 *     with their own register contracts (d6/a6 swapped with EXG, a5/a6
 *     kept around the debugger's cleanup), there is nothing for C to do;
 *   - byte identity includes the relocation table.  The original (made
 *     by Blitz 2) lists first the references to its own labels in
 *     ascending order (with the last ALibJsr, globfree in codefinit,
 *     among them), then the other ALibJsr placeholders in descending
 *     order.  ld keeps the order of the object file, where gas groups the
 *     relocations by target (section, then external symbol) in the order
 *     they were made.  So: code and data are in the section of the token
 *     table (.blitzlib.head; with the code in .text the header's
 *     relocations would come out as code first, then table entries), the
 *     last ALibJsr is relative to dh_hunk0 (offset 0 of the section =
 *     offset 0 of the hunk, like _blitz_tok) and the other placeholders
 *     are absolute operands with an explicit .reloc against _blitz_tok,
 *     written in descending order at the end.
 * The result is byte-identical to the CD's dhandlerlib.obj.
 */
#include "blitzlib.h"

/**
 * @brief Library init (assembler below): starts the debugger program as
 *        the process "Blitz ][ Debug Proc".
 *
 * Copies the code template mystart..mystart_f (memlib globalloc, not
 * checked), patches it with ltable and progadd and starts it with
 * CreateProc; waits (WaitTOF) until the debugger has called l2
 * (dosethandler), then installs the Ctrl-LAlt-C check as vertical blank
 * server (intlib SetInt, level 5).  Nothing happens without a debugger
 * program (d0 = 0); waits for ever if it never calls l2.
 * BUG(orig) (#409): the copied code is started without a cache flush.
 *
 * In: d0 = BPTR of the debugger program loaded by the editor (0: none),
 * a6 = dos, d6 = graphics.
 */
void dhandlerlib_codeinit(void);
/**
 * @brief Library finit (assembler below): ends the debugger process.
 *
 * Removes the vertical blank server, calls the debugger's cleanup handler
 * (handler table +20), waits until the process has returned (taskdone)
 * and frees the segment.
 * BUG(orig) (#410): the segment is freed while the process still has to
 * execute its last RTS in it.
 *
 * In: a6 = dos, d6 = graphics.
 */
void dhandlerlib_codefinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* offset 0 of .blitzlib.head (this library is the only object there),
   for the ALibJsr placeholder that is a reference to the library's own
   section in the original's relocation table (see above) */
__asm__("   .section .blitzlib.head\ndh_hunk0:\n");

/* !libheader{#dhandlerlib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_dhandlerlib, init, BL_NORET, finit, 0);
/* init:!nullsub{codeinit,0,0,#doslib,#la6,#graphicslib,#ld6} */
BLITZ_NULLSUB_AT(init, dhandlerlib_codeinit, 0, 0,
                 LIB_doslib, BL_LA(6), LIB_graphicslib, BL_LD(6));
/* finit:!nullsub{codefinit,0,0,#doslib,#la6,#graphicslib,#ld6} */
BLITZ_NULLSUB_AT(finit, dhandlerlib_codefinit, 0, 0,
                 LIB_doslib, BL_LA(6), LIB_graphicslib, BL_LD(6));
BLITZ_LIBFIN();

/* debuglib's tokens (debuglib.c): donewmode $D500 ... doskip $D508 */
#define TOK_debuglib    0xd500
#define TOK_dostop      0xd501
#define TOK_dosethandler 0xd502
/* blitzkeyslib RawStatus (d0 = raw key code -> d0 = 0: up) */
#define TOK_rawstatus   ((LIB_blitzkeyslib << 7) + 2)

/* ALibJsr placeholder of token `tok` as an absolute operand (relocation:
   .reloc at the end) */
#define DH_JSR(tok) "jsr     0x80000000+" BL_STR(tok) "\n"
#define DH_DJSR(n) "jsr     0x80000000+" BL_STR(TOK_debuglib) "+" #n "\n"

/* dos.library / graphics.library LVOs */
#define LVO_CreateProc  (-138)
#define LVO_WaitTOF     (-270)

__asm__(
"   .section .blitzlib.head\n"
"   .even\n"
"taskstack:  .long   4096\n"
"handler:    .long   0\n"
"segadd:     .long   0\n"
"progadd:    .long   0\n"
"mysegadd:   .long   0\n"
"taskdone:   .word   0\n"
"mysegname:  .asciz  \"Blitz ][ Debug Proc\"\n"
"   .even\n"
/* the process's segment: next segment (BPTR 0), then the code.  The
   operands of off_1 (#ltable) and off_2 (the address of progadd) are
   0 here and patched in the copy, so both stay absolute longs. */
"mystart:    .long   0\n"
"   move.l  (4).l,a0\n"
"   move.l  276(a0),a0\n"               /* ThisTask */
"off_1:\n"
"   move.l  #0,88(a0)\n"                /* tc_UserData = ltable */
"off_2:\n"
"   move.l  (0).l,a0\n"                 /* progadd */
"   jsr     (a0)\n"
"   st      taskdone\n"
"   rts\n"
"mystart_f:\n"
"\n"
"ltable:\n"
"   .long   l0,l1,l2,l3,l4,l5,l6,l7,l8\n"
"l0:\n"
"j_l0: " DH_DJSR(0)
"   rts\n"
"l1:\n"
"j_l1: " DH_DJSR(1)
"   rts\n"
"l2: move.l a0,handler\n"
"j_l2: " DH_DJSR(2)
"   rts\n"
"l3:\n"
"j_l3: " DH_DJSR(3)
"   rts\n"
"l4:\n"
"j_l4: " DH_DJSR(4)
"   rts\n"
"l5:\n"
"j_l5: " DH_DJSR(5)
"   rts\n"
"l6:\n"
"j_l6: " DH_DJSR(6)
"   rts\n"
"l7:\n"
"j_l7: " DH_DJSR(7)
"   rts\n"
"l8:\n"
"j_l8: " DH_DJSR(8)
"   rts\n"
"\n"
/* codeinit - d0 = BPTR of the debugger program (0: none), a6 = dos,
   d6 = graphics.  The result of globalloc is not checked: memlib ends
   the program with "Out of Memory!" itself.  Waits for ever if the
   debugger program never calls l2.
   BUG(orig) (#409): the copied code is started without a cache flush
   (CacheClearU): with instruction cache (68020+), and above all with
   the copyback data cache of the 68040/060, the process may execute what
   was in that memory before. */
"   .globl  _dhandlerlib_codeinit\n"
"_dhandlerlib_codeinit:\n"
"   clr.w   taskdone\n"
"   clr.l   handler\n"
"   move.l  d0,segadd\n"
"   beq.w   9f\n"
"   lsl.l   #2,d0\n"
"   addq.l  #4,d0\n"
"   move.l  d0,progadd\n"
"   moveq   #mystart_f-mystart,d0\n"
"   moveq   #1,d1\n"                    /* MEMF_PUBLIC */
"j_alloc: " DH_JSR(TOK_globalloc)
"   move.l  d0,mysegadd\n"
"   lea     mystart(pc),a0\n"
"   move.l  d0,a1\n"
"1: move.w  (a0)+,(a1)+\n"
"   cmp.l   #mystart_f,a0\n"
"   bcs.w   1b\n"
"   move.l  d0,a1\n"
"   move.l  #ltable,off_1-mystart+2(a1)\n"
"   move.l  #progadd,off_2-mystart+2(a1)\n"
"   move.l  #mysegname,d1\n"
"   moveq   #0,d2\n"                    /* priority */
"   move.l  d0,d3\n"
"   lsr.l   #2,d3\n"                    /* seglist BPTR */
"   move.l  taskstack(pc),d4\n"
"   jsr     " BL_STR(LVO_CreateProc) "(a6)\n"
"   exg     d6,a6\n"
"2: tst.l   handler\n"
"   bne.w   3f\n"
"   jsr     " BL_STR(LVO_WaitTOF) "(a6)\n"
"   bra.w   2b\n"
"3: exg     d6,a6\n"
"   move.w  #0x8035,d0\n"
"   move.l  #check_c,d1\n"
"j_setint: " DH_JSR(TOK_setint)
"9: rts\n"
"\n"
/* check_c - vertical blank server: Ctrl ($63), LAlt ($64), C ($33) */
"check_c:\n"
"   moveq   #0x63,d0\n"
"j_raw1: " DH_JSR(TOK_rawstatus)
"   beq.w   9f\n"
"   moveq   #0x64,d0\n"
"j_raw2: " DH_JSR(TOK_rawstatus)
"   beq.w   9f\n"
"   moveq   #0x33,d0\n"
"j_raw3: " DH_JSR(TOK_rawstatus)
"   beq.w   9f\n"
"   tst.w   laststop\n"
"   beq.w   8f\n"
"   clr.w   laststop\n"
"j_stop: " DH_JSR(TOK_dostop)
"   moveq   #0,d0\n"
"   rts\n"
"9: st      laststop\n"
"   moveq   #0,d0\n"
"8: rts\n"
"laststop:   .word   0\n"
"\n"
/* codefinit - a6 = dos, d6 = graphics.
   BUG(orig) (#410): the segment is freed as soon as taskdone is set, but
   the process still has to execute the RTS behind ST taskdone in it (a
   small window: codefinit only looks again after a WaitTOF). */
"   .globl  _dhandlerlib_codefinit\n"
"_dhandlerlib_codefinit:\n"
"   move.l  segadd(pc),d1\n"
"   beq.w   9f\n"
"   move.w  #0x8035,d0\n"
"j_clrint: " DH_JSR(TOK_clrint)
"   movem.l a5-a6,-(a7)\n"
"   move.l  handler(pc),a0\n"
"   move.l  20(a0),a0\n"
"   jsr     (a0)\n"                     /* tell him to stop! */
"   movem.l (a7)+,a5-a6\n"
"   exg     a6,d6\n"                    /* wait for task to be removed! */
"1: tst.w   taskdone\n"
"   bne.w   2f\n"
"   jsr     " BL_STR(LVO_WaitTOF) "(a6)\n"
"   bra.w   1b\n"
"2: exg     a6,d6\n"
"   move.l  mysegadd(pc),a1\n"
"   moveq   #mystart_f-mystart,d0\n"
"   jsr     dh_hunk0+0x80000000+" BL_STR(TOK_globfree) "\n"
"9: rts\n"
/* the end of the program (Blitz 2 appends it to every program) */
"   moveq   #0,d0\n"
"   rts\n"
"\n"
/* the relocations of the other ALibJsr placeholders, in the original's
   order */
"   .reloc  j_clrint+2,BFD_RELOC_32,_blitz_tok\n"
"   .reloc  j_stop+2,BFD_RELOC_32,_blitz_tok\n"
"   .reloc  j_raw3+2,BFD_RELOC_32,_blitz_tok\n"
"   .reloc  j_raw2+2,BFD_RELOC_32,_blitz_tok\n"
"   .reloc  j_raw1+2,BFD_RELOC_32,_blitz_tok\n"
"   .reloc  j_setint+2,BFD_RELOC_32,_blitz_tok\n"
"   .reloc  j_alloc+2,BFD_RELOC_32,_blitz_tok\n"
"   .reloc  j_l8+2,BFD_RELOC_32,_blitz_tok\n"
"   .reloc  j_l7+2,BFD_RELOC_32,_blitz_tok\n"
"   .reloc  j_l6+2,BFD_RELOC_32,_blitz_tok\n"
"   .reloc  j_l5+2,BFD_RELOC_32,_blitz_tok\n"
"   .reloc  j_l4+2,BFD_RELOC_32,_blitz_tok\n"
"   .reloc  j_l3+2,BFD_RELOC_32,_blitz_tok\n"
"   .reloc  j_l2+2,BFD_RELOC_32,_blitz_tok\n"
"   .reloc  j_l1+2,BFD_RELOC_32,_blitz_tok\n"
"   .reloc  j_l0+2,BFD_RELOC_32,_blitz_tok\n"
);
