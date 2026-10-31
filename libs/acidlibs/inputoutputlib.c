/*
 * inputoutputlib - command library 164: DefaultInput DefaultOutput
 * PopInput PopOutput and the input/output channel routines that the other
 * libraries call (printlib, editlib, fileiolib, windowslib ...).
 *
 * C translation of bblibsrc/inputoutputlib ("v1.1 - added popinput and
 * popoutput"); binary of the CD's acidlibs checked: same as the source.
 *
 * Channels: the current input and output are "handlers", code addresses
 * with three entries:
 *      handler-8   output: null terminated string at a0
 *      handler-4   output: string a0, length d0 / input: read at most d0
 *                  characters to a0, d0 = number read
 *      handler     output: character d0 / input: d0 = character (<0 none)
 * (each entry before the handler is a BRA.w).  The routines for other
 * libraries (tokens ALibJsr #inputoutputlib LSL 7+n):
 *      +3 dosetin, +4 dosetout   a0 = handler -> d0 = previous handler,
 *                                which is pushed on a stack of 16
 *      +5 dogetsingle            JSR (incode)
 *      +6 dogetstring            JSR -4(incode)
 *      +7 dooutsingle            JSR (outcode)
 *      +8 dooutstring1           JSR -4(outcode)
 *      +9 dooutstring2           JSR -8(outcode)
 * The five dispatchers stay assembler (the original instructions): they
 * hand ALL registers unchanged to the foreign handler, with a6 = the
 * handler address, and return whatever the handler returns; only a6 is
 * restored.  A C function in between could not promise that.
 *
 * Default channels (dos.library Input()/Output(), read once by init):
 *      defout   Write of 1 character / of d0 characters / of a null
 *               terminated string (nothing for an empty one)
 *      defin    -4: Read(Input(), a0, d0+1) - 1 (the last character, the
 *               newline of a console line, is dropped; d0 = -1 at end of
 *               file, -2 on an error);  0: no single characters - runtime
 *               error "Can't Input Character from Defualt Input" (sic,
 *               BUG(orig) (#193): typo kept)
 *
 * Stacks: DefaultInput/DefaultOutput and every dosetin/dosetout push the
 * previous handler (word index & 63: a ring of 16 longs) and count;
 * PopInput/PopOutput go back one entry.  init pushes the initial handler
 * 0 on both stacks, so after one PopOutput at the start the output
 * handler is 0.  Without the debugger nothing is checked (popping more
 * than was pushed takes old ring entries, BUG(orig) (#194)).
 *
 * DIFF: in the original the single character entry of defin is `BRA err1`
 * into the error check area: with the debugger the runtime error, without
 * the debugger a jump behind the library code (the error area is not in
 * the program).  The C library must not reference the error check area
 * from normal code (README: check routines), so the trap and its message are in
 * the normal code: the runtime error comes also without the debugger.
 *
 * Registers (thunks keep at least what the original keeps): handlers
 * defout d1-d3/a0-a1, defout2/defin2 d1-d3/a1, defout3 d0-d3/a1 (C keeps
 * d2-d7/a2-a6 and the thunks d1/a0-a1 resp. d0-d1/a0-a1); unlike the
 * original they do not leave a6 = dos base (the dispatchers restore a6
 * anyway).  dosetin/dosetout/pops keep d1/a1 (and a0).
 *
 * Error checks (debugger only): PopInput/PopOutput with count 0 -> "No
 * Input channel to pop!" / "No Output channel to pop!".
 */
#include "blitzlib.h"
#define __NOLIBBASE__
#include <proto/dos.h>
#undef __NOLIBBASE__

/* entry points (thunks / assembler, defined below) */
void dodefin(void), dodefout(void), dosetin(void), dosetout(void);
void dogetsingle(void), dogetstring(void), dooutsingle(void);
void dooutstring1(void), dooutstring2(void), dopopinput(void);
void dopopoutput(void), codeinit(void);
void popinputcheck(void), popoutputcheck(void);

BLITZ_ENTRY_DECL(init);

/* !libheader{#inputoutputlib,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_inputoutputlib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dodefin, 0, 0);
BLITZ_NAME("DefaultInput", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dodefout, 0, 0);
BLITZ_NAME("DefaultOutput", "");

BLITZ_NULLSUB(dosetin, 0, 0);
BLITZ_NULLSUB(dosetout, 0, 0);
BLITZ_NULLSUB(dogetsingle, 0, 0);
BLITZ_NULLSUB(dogetstring, 0, 0);
BLITZ_NULLSUB(dooutsingle, 0, 0);
BLITZ_NULLSUB(dooutstring1, 0, 0);
BLITZ_NULLSUB(dooutstring2, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dopopinput, popinputcheck, 0);
BLITZ_NAME("PopInput", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dopopoutput, popoutputcheck, 0);
BLITZ_NAME("PopOutput", "");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0, LIB_doslib, BL_LA(6));

BLITZ_LIBFIN();

/* ------------------------------------------------------------ variables */

/* one channel: the current handler and the stack of previous ones */
struct channel {
    void *code;             /* incode / outcode */
    ULONG stack[16];        /* remember up to 16 handlers! */
    UWORD sp;               /* byte offset, & 63 */
    UWORD count;
};

/* not static: the dispatchers (assembler) load iochan_in/iochan_out.code */
struct channel iochan_in, iochan_out;
static struct DosLibrary *dosbase;
static BPTR inputh, outputh;
static UBYTE outchar;                           /* temp: defout's character */

#define DOSBase dosbase                         /* for proto/dos.h */

/* --------------------------------------------------- set / pop channel */

/* set the handler, push the previous one, return it */
static ULONG setchannel(struct channel *c, void *handler)
{
    ULONG old = (ULONG)c->code;

    c->code = handler;
    *(ULONG *)((UBYTE *)c->stack + (WORD)c->sp) = old;  /* MOVE.l d0,0(a1,d1) */
    c->sp = (c->sp + 4) & 63;
    c->count++;
    return old;
}

static void popchannel(struct channel *c)
{
    c->count--;
    c->sp = (c->sp - 4) & 63;
    c->code = *(void **)((UBYTE *)c->stack + (WORD)c->sp);
}

static ULONG BLITZ_ROUTINE dosetin_c(void *handler BL_REG(a0))
{
    return setchannel(&iochan_in, handler);
}
BLITZ_THUNK(dosetin, dosetin_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE dosetout_c(void *handler BL_REG(a0))
{
    return setchannel(&iochan_out, handler);
}
BLITZ_THUNK(dosetout, dosetout_c, "d1/a0-a1");

static void BLITZ_ROUTINE dopopinput_c(void)
{
    popchannel(&iochan_in);
}
BLITZ_THUNK(dopopinput, dopopinput_c, "d1/a0-a1");

static void BLITZ_ROUTINE dopopoutput_c(void)
{
    popchannel(&iochan_out);
}
BLITZ_THUNK(dopopoutput, dopopoutput_c, "d1/a0-a1");

/* ------------------------------------------------------ default output */

/* defout - single character d0 (Write of 1 byte from outchar) */
static LONG BLITZ_ROUTINE defout_c(ULONG c BL_REG(d0))
{
    outchar = (UBYTE)c;
    return Write(outputh, &outchar, 1);
}

/* defout2 - string a0, length d0 */
static LONG BLITZ_ROUTINE defout2_c(ULONG len BL_REG(d0), const UBYTE *s BL_REG(a0))
{
    return Write(outputh, (APTR)s, len);
}

/* defout3 - null terminated string a0 (TST d3: an empty string, or one
   whose length is a multiple of 65536, writes nothing) */
static void BLITZ_ROUTINE defout3_c(const UBYTE *s BL_REG(a0))
{
    const UBYTE *p = s;
    ULONG n = (ULONG)-1;

    do
        n++;
    while (*p++);
    if ((UWORD)n)
        Write(outputh, (APTR)s, n);
}

void defout(void);
__asm__("   .text\n   .even\n"
        "   bra.w   2f\n"                       /* -8: defout3 */
        "   bra.w   1f\n"                       /* -4: defout2 */
        "   .globl  _defout\n"
        "_defout:\n"
        "   movem.l d1/a0-a1,-(sp)\n"
        "   jsr     _defout_c\n"
        "   movem.l (sp)+,d1/a0-a1\n"
        "   rts\n"
        "1: movem.l d1/a0-a1,-(sp)\n"
        "   jsr     _defout2_c\n"
        "   movem.l (sp)+,d1/a0-a1\n"
        "   rts\n"
        "2: movem.l d0-d1/a0-a1,-(sp)\n"
        "   jsr     _defout3_c\n"
        "   movem.l (sp)+,d0-d1/a0-a1\n"
        "   rts\n");

/* ------------------------------------------------------- default input */

/* defin2 - read at most d0+1 characters to a0, d0 = number - 1 */
static LONG BLITZ_ROUTINE defin2_c(ULONG len BL_REG(d0), UBYTE *buf BL_REG(a0))
{
    return Read(inputh, buf, len + 1) - 1;
}

/* message of defin's single character entry - in the normal code (DIFF,
   see the head comment; the original's text is in the error area).
   BUG(orig) (#193): "Defualt" - the typo of the original, kept */
const char mess1[] = "Can't Input Character from Defualt Input";

void defin(void);
__asm__("   .text\n   .even\n"
        "   bra.w   1f\n"                       /* -4: defin2 */
        "   .globl  _defin\n"
        "_defin:\n"                             /*  0: BRA err1 */
        "   move.l  #_mess1,d0\n"
        "   trap    #0\n"
        "1: movem.l d1/a0-a1,-(sp)\n"
        "   jsr     _defin2_c\n"
        "   movem.l (sp)+,d1/a0-a1\n"
        "   rts\n");

/* ------------------------------------------------ DefaultInput/Output */

static ULONG BLITZ_ROUTINE dodefin_c(void)
{
    return setchannel(&iochan_in, defin);
}
BLITZ_THUNK(dodefin, dodefin_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE dodefout_c(void)
{
    return setchannel(&iochan_out, defout);
}
BLITZ_THUNK(dodefout, dodefout_c, "d1/a0-a1");

/* init (a6 = dos.library): remember the dos base and Input()/Output(),
   empty stacks, then DefaultInput and DefaultOutput (each pushes the
   handler 0) */
static void BLITZ_ROUTINE codeinit_c(struct DosLibrary *dos BL_REG(a6))
{
    dosbase = dos;
    inputh = Input();
    iochan_in.sp = 0;
    iochan_in.count = 0;
    outputh = Output();
    iochan_out.sp = 0;
    iochan_out.count = 0;
    setchannel(&iochan_in, defin);
    setchannel(&iochan_out, defout);
}
BLITZ_THUNK(codeinit, codeinit_c, "d0-d1/a0-a1");

/* ------------------------------------------------- channel dispatchers */

/* MOVE.l a6,-(a7):MOVE.l code(pc),a6:JSR off(a6):MOVE.l (a7)+,a6:RTS -
   assembler: all registers go unchanged to the handler (see the head
   comment) */
#define DISPATCH(entry, code, off)                                          \
    BLITZ_ASM_ROUTINE(entry,                                                \
        "   move.l  a6,-(sp)\n"                                             \
        "   move.l  " code ",a6\n"                                          \
        "   jsr     " off "(a6)\n"                                          \
        "   move.l  (sp)+,a6\n"                                             \
        "   rts")

DISPATCH(dogetsingle, "_iochan_in", "0");
DISPATCH(dogetstring, "_iochan_in", "-4");
DISPATCH(dooutsingle, "_iochan_out", "0");
DISPATCH(dooutstring1, "_iochan_out", "-4");
DISPATCH(dooutstring2, "_iochan_out", "-8");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess2, "No Input channel to pop!");
BLITZ_ERRCHK_MESSAGE(mess3, "No Output channel to pop!");

/* popinputcheck - TST inputcount:BEQ err2 */
static void BLITZ_ERRCHK popinputcheck_c(void)
{
    if (!iochan_in.count)
        BLITZ_RUNERR(mess2);
}

static void BLITZ_ERRCHK popoutputcheck_c(void)
{
    if (!iochan_out.count)
        BLITZ_RUNERR(mess3);
}

/* the thunks after all check functions: they leave the assembler in .text
   while GCC still assumes the error check section */
BLITZ_ERRCHK_THUNK(popinputcheck, popinputcheck_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(popoutputcheck, popoutputcheck_c, "d0-d1/a0-a1");
