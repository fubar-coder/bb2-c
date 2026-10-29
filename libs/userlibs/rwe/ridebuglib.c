/*
 * ridebuglib - RWE userlib 51 (Leading Edge Software, Steven Matty, V2.1
 * of 27/2/95): talking to the Blitz debugger - AddVarTrace, DelVarTrace,
 * VarTraceWindow, DisAsmWindow, CopperTrace, ProcControl, MemoryWindow.
 *
 * C translation of rwelibsrc/Debuglib/Debuglib4.bb; reference OL
 * RIDEBUGLIB.OBJ (the reference build of the source equals it byte for
 * byte, docs/rwelibs/README.md 3.1; BSS2's RIDEBUGLIB.OBJ is another
 * build).  Notes: docs/rwelibs/port/ridebuglib.md.
 *
 * Every command's routine is an RTS (Amiga mode only, also init and
 * finit); the work is done in its ERROR CHECK routine, so the commands act
 * only when the program is compiled with the debugger - without it the
 * compiler cuts the error check area away.  A check fills the message
 * block mess1 and hands it to the debugger through debuglib's docall
 * ($D509, a0 = message, handler 6 of the front end):
 *   +0 command.b, +1 data1.b, +2 data2.b, +3 pad.b, +4 size.l (an address
 *   since 5/1/94), +8 252 characters.
 * Bytes a command does not set keep what the previous one left.
 *
 * C, with one exception: DisAsmWindow's check needs the return address of
 * its caller (the program, (a7) in the original), which the thunk passes
 * in a0 - a thunk of its own in assembler (BLITZ_THUNK cannot load it).
 * The original's `error` routine (MOVE.l #mess1,d7:TRAP #1, called by
 * nothing) is left out; the header's error field is the start of the C
 * error check area.
 *
 * BUG(orig):
 *   - AddVarTrace/DelVarTrace copy the name with a loop that tests the
 *     byte AFTER the one copied: an empty name copies its 0 and then
 *     whatever follows up to the next 0.  Nothing limits the copy to the
 *     252 characters of mess1.
 *   - AddVarTrace converts the type word with an 8 byte table, indexed
 *     without a check (other types read the code behind the table; the C
 *     build reads other bytes there, DIFF).
 *   - DelVarTrace writes the name at +12, not at +8 where the characters
 *     start (+1..+11 keep the previous message's bytes), and its 0 without
 *     advancing.
 *   - DisAsmWindow searches backwards from the return address for a word
 *     $4E41 (TRAP #1, which the debugger's code has before the call) with
 *     no limit.
 */
#include "blitzlib.h"

/* debuglib docall: a0 = the message, to the debugger's handler 6 */
#define TOK_docall 0xd509

/* the debugger's commands (#_addtrace ...) */
#define DC_ADDTRACE      255
#define DC_DELTRACE      254
#define DC_VARWINDOW     253
#define DC_DISWINDOW     252
#define DC_COPPERLIST    251
#define DC_PROCCONTROL   250
#define DC_MEMWINDOWMOVE 249

/* entry points (thunks, below) */
void _dummybranch(void);
void _addvartracechk(void), _delvartracechk(void), _vartracewindowchk(void);
void _disasmchk(void), _coptracechk(void), _coptracechk2(void);
void _procstatus(void), _procstatus2(void), _memadrchk(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#ridebuglib,init,0,finit,error} */
BLITZ_LIBHEADER(LIB_ridebuglib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_UNKNOWN | BL_VARPTR, BL_STRING, BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_dummybranch), _addvartracechk, 0);
BLITZ_NAME("AddVarTrace", "variable,string$,output - add a variable trace");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_dummybranch), _delvartracechk, 0);
BLITZ_NAME("DelVarTrace", "variable$ - remove a trace on a named variable");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_dummybranch), _vartracewindowchk, 0);
BLITZ_NAME("VarTraceWindow", "- open the variable trace window on the debugger screen");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_dummybranch), _disasmchk, 0);
BLITZ_NAME("DisAsmWindow", "- open the disassembly window on the debugger screen");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_dummybranch), _coptracechk, 0);
BLITZ_ARGS(BL_LONG, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_dummybranch), _coptracechk2, 0);
BLITZ_NAME("CopperTrace", "address,[,offset] - offset<>0 for CopList objects");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_dummybranch), _procstatus, 0);
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_dummybranch), _procstatus2, 0);
BLITZ_NAME("ProcControl", "[On/Off]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_dummybranch), _memadrchk, 0);
BLITZ_NAME("MemoryWindow", "num#,address - open a memory window on the debugger screen");

BLITZ_NULLSUB_AT(finit, _dummybranch, 0, 0);
BLITZ_NULLSUB_AT(init, _dummybranch, 0, 0);

BLITZ_LIBFIN();

/* _dummybranch: RTS - every routine, init and finit */
void BLITZ_ROUTINE _dummybranch(void)
{
}

/* --------------------------------------------- the checks (debugger only) */

/* mess1: the message block (command, data1, data2, pad, size.l, 252
   characters).  The original has it in the error check area; here it is
   ordinary data, 260 bytes that also stay without the debugger. */
static UBYTE mess1[4 + 4 + 252] __attribute__((aligned(2)));

#define SETL(p, v) (*(ULONG *)(p) = (v))

/* LEA mess1(pc),a0:ALibJsr $d509 */
static void BLITZ_ERRCHK send(void)
{
    struct bl_regs r = { .a0 = (ULONG)mess1 };

    BLITZ_ALIBJSR(TOK_docall, r);
}

/* 'converttype: Blitz type -> debugger type */
static const UBYTE converttype[8] = { 0, 1, 2, 3, 5, 6, 0, 4 };

/* the name copy of AddVarTrace/DelVarTrace: tests the byte after the one
   copied (BUG(orig)); returns the end */
static UBYTE *BLITZ_ERRCHK copyname(UBYTE *d, const UBYTE *s)
{
    do
        *d++ = *s++;
    while (*s);
    return d;
}

/* AddVarTrace variable,name$,output: d0 = varptr, d1 = name, d2 =
   output, -2(a2) = the type word */
static void BLITZ_ERRCHK addvartrace_c(ULONG var BL_REG(d0), const UBYTE *name BL_REG(d1),
                                       UWORD out BL_REG(d2), const UWORD *types BL_REG(a2))
{
    UBYTE type = converttype[(WORD)types[-1]];          /* 0(a1,d0.w), BUG(orig) */
    UBYTE *p;

    mess1[0] = DC_ADDTRACE;
    mess1[1] = type ? type : 1;
    mess1[2] = (UBYTE)out ? (UBYTE)out : 1;
    mess1[3] = 255;
    SETL(mess1 + 4, var);
    p = copyname(mess1 + 8, name);
    *p = 0;
    send();
}

/* DelVarTrace name$: d0 = name, written at +12 (BUG(orig)) */
static void BLITZ_ERRCHK delvartrace_c(const UBYTE *name BL_REG(d0))
{
    mess1[0] = DC_DELTRACE;
    *copyname(mess1 + 12, name) = 0;
    send();
}

/* VarTraceWindow */
static void BLITZ_ERRCHK vartracewindow_c(void)
{
    mess1[0] = DC_VARWINDOW;
    mess1[1] = 0xff;
    send();
}

/* DisAsmWindow: a0 = the return address into the program (thunk); back
   to the TRAP #1 in front of the call */
static void BLITZ_ERRCHK disasm_c(const UWORD *pc BL_REG(a0))
{
    while (*pc != 0x4e41)                               /* BUG(orig): no limit */
        pc--;
    mess1[0] = DC_DISWINDOW;
    mess1[1] = 0xff;
    mess1[2] = 0xff;
    mess1[3] = 0xff;
    SETL(mess1 + 4, (ULONG)pc);
    mess1[8] = 0;
    send();
}

/* CopperTrace address: if not 0 */
static void BLITZ_ERRCHK coptrace_c(ULONG adr BL_REG(d0))
{
    if (!adr)
        return;
    mess1[0] = DC_COPPERLIST;
    mess1[1] = 0;
    mess1[2] = 0;
    mess1[3] = 0;
    SETL(mess1 + 4, adr);
    mess1[8] = 0;
    send();
}

/* CopperTrace address,offset: the copper list at address+offset.w (a
   CopList object) */
static void BLITZ_ERRCHK coptrace2_c(const UBYTE *adr BL_REG(d0), UWORD off BL_REG(d1))
{
    if (!adr)
        return;
    coptrace_c(*(const ULONG *)(adr + (WORD)off));
}

/* ProcControl [on]: data1 = on (toggle: 0), data2 = 0 (toggle) / $FF */
static void BLITZ_ERRCHK procstatus_c(UBYTE on, UBYTE explicit)
{
    mess1[2] = explicit;
    mess1[0] = DC_PROCCONTROL;
    mess1[1] = on;
    mess1[4] = 0;
    send();
}

static void BLITZ_ERRCHK procstatus0_c(void)
{
    procstatus_c(0, 0);
}

static void BLITZ_ERRCHK procstatus2_c(UWORD on BL_REG(d0))
{
    procstatus_c((UBYTE)on, 0xff);
}

/* MemoryWindow num,address: data1 = num.b, size = address */
static void BLITZ_ERRCHK memadr_c(UWORD num BL_REG(d0), ULONG adr BL_REG(d1))
{
    mess1[0] = DC_MEMWINDOWMOVE;
    mess1[1] = (UBYTE)num;
    SETL(mess1 + 4, adr);
    mess1[8] = 0;
    send();
}

/* Registers: the original's checks change a0/a1 (and d0, d3, AddVarTrace
   also d2 and a2, ProcControl d0.b) and whatever docall changes; the
   thunks keep d0/d1/a0/a1, the C code the rest. */
BLITZ_ERRCHK_THUNK(_addvartracechk, addvartrace_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_delvartracechk, delvartrace_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_vartracewindowchk, vartracewindow_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_coptracechk, coptrace_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_coptracechk2, coptrace2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_procstatus, procstatus0_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_procstatus2, procstatus2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_memadrchk, memadr_c, "d0-d1/a0-a1");

/* DisAsmWindow's check: the return address of the call (MOVE.l (a7),a0
   in the original) is above the saved registers - assembler, a C
   function only sees the thunk's own return address */
__asm__("   .section .blitzlib.errchk,\"ax\"\n"
        "   .even\n   .globl  __disasmchk\n"
        "__disasmchk:\n"
        "   movem.l d0-d1/a0-a1,-(sp)\n"
        "   move.l  16(sp),a0\n"
        "   jsr     _disasm_c\n"
        "   movem.l (sp)+,d0-d1/a0-a1\n"
        "   rts\n"
        "   .text\n");
