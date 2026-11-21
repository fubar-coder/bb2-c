/*
 * patchflagstest - proves that a PB_THRU patch (patchblock.h, the patches of
 * inject) hands back the condition codes of the stub it called: a caller
 * may branch on them right after the call (ciatrklib's `jsr Examine ; beq`).
 *
 *   patchflagstest [old]
 *
 * dos Delay() is patched with the THRU template; the stub is a few
 * instructions that load known values into d0, d1, a0, a1 and set the
 * condition codes from d2.  The caller sets d2, calls the vector and
 * records the four flags with Scc (no instruction between the call and
 * the Scc changes them) and the registers.  One line per check
 * ("patchflagstest: NAME: expected" or "MISMATCH"), then
 * "patchflagstest: OK" (exit status 0) or "FAIL" (5):
 *
 *   flags 00 .. 0F   C, V, Z and N after the call are those the stub set
 *                    (the CCR values 00, 01, 02, 04, 08, 0F)
 *   regs             d0, d1, a0, a1 and a6 are those the stub left
 *   removed          no call inside after the removal, the vector is back
 *
 * With `old` the patch is installed from a template with the return path of
 * the trampoline before the flags were kept (it ends in `subq.l #1,(a0)`:
 * the flags of the count, Z set): the broken input of the test, the flags
 * lines with Z clear must fail ("MISMATCH"), the registers still hold.
 */
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

#define PB_THRU
#include "patchblock.h"

/* the trampoline before its return path kept the condition codes */
__asm__(
PB_ASM_HEAD(thruold, "")
"   move.l  a0,-(sp)\n"
"   lea     _pb_thruold_tmpl(pc),a0\n"
"   addq.l  #1,(a0)\n"
"   tst.l   4(a0)\n"
"   beq.s   1f\n"
"   move.l  (sp)+,a0\n"
"   pea     2f(pc)\n"
"   move.l  _pb_thruold_tmpl+12(pc),-(sp)\n"
"   rts\n"
"2: move.l  a0,-(sp)\n"
"   lea     _pb_thruold_tmpl(pc),a0\n"
"   subq.l  #1,(a0)\n"
"   move.l  (sp)+,a0\n"
"   rts\n"
"1: subq.l  #1,(a0)\n"
"   move.l  (sp)+,a0\n"
"   move.l  _pb_thruold_tmpl+8(pc),-(sp)\n"
"   rts\n"
"_pb_thruold_end:\n"
);
PB_DECLARE(thruold);

#define STUB_D0 0x12345678UL
#define STUB_D1 0x0D1D1D1DUL
#define STUB_A0 0x0A0A0A0AUL
#define STUB_A1 0x0A1A1A1AUL

/* the stub: values into the registers, the flags from d2 last (a plain
   function for the trampoline, ended by rts) */
__asm__(
"   .text\n"
"   .even\n"
"_pbflags_stub:\n"
"   move.l  #0x12345678,d0\n"
"   move.l  #0x0D1D1D1D,d1\n"
"   move.l  #0x0A0A0A0A,a0\n"
"   move.l  #0x0A1A1A1A,a1\n"
"   move.w  d2,ccr\n"
"   rts\n"
);
extern void pbflags_stub(void) __asm__("_pbflags_stub");

/* out[0..3] = d0 d1 a0 a1, out[4] = a6, then the flags C V Z N as four
   bytes (ff = set); the call is Delay's vector with d2 = the CCR for the
   stub */
__asm__(
"   .text\n"
"   .even\n"
"_pbflags_call:\n"
"   movem.l d2-d7/a2-a6,-(sp)\n"
"   move.l  48(sp),a6\n"
"   move.l  52(sp),d2\n"
"   move.l  56(sp),a2\n"
"   jsr     -198(a6)\n"
"   scs     d3\n"                   /* the Scc leave the flags alone */
"   svs     d4\n"
"   seq     d5\n"
"   smi     d6\n"
"   move.l  d0,(a2)+\n"
"   move.l  d1,(a2)+\n"
"   move.l  a0,(a2)+\n"
"   move.l  a1,(a2)+\n"
"   move.l  a6,(a2)+\n"
"   move.b  d3,(a2)+\n"
"   move.b  d4,(a2)+\n"
"   move.b  d5,(a2)+\n"
"   move.b  d6,(a2)+\n"
"   movem.l (sp)+,d2-d7/a2-a6\n"
"   rts\n"
);
extern void pbflags_call(struct Library *lib, ULONG ccr, UBYTE *out) __asm__("_pbflags_call");

static int bad;

static void put(const char *s)
{
    Write(Output(), (APTR)s, strlen(s));
}

static void line(const char *name, int ok)
{
    put("patchflagstest: ");
    put(name);
    put(ok ? ": expected\n" : ": MISMATCH\n");
    if (!ok) bad = 1;
}

static ULONG rd(const UBYTE *p)
{
    return ((ULONG)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

int main(int argc, char **argv)
{
    static const UBYTE ccrs[] = { 0x00, 0x01, 0x02, 0x04, 0x08, 0x0F };
    static const char *const names[] = { "flags 00", "flags 01", "flags 02", "flags 04", "flags 08", "flags 0F" };
    int old = argc > 1 && !strcmp(argv[1], "old"), i, regs_ok = 1;
    struct pbhead *blk;
    UBYTE out[24];
    LONG left;
    APTR vec0 = *(APTR *)((UBYTE *)DOSBase + -198 + 2);

    if (old)
        blk = PB_INSTALL((struct Library *)DOSBase, -198, thruold, sizeof(struct pbhead), pbflags_stub);
    else
        blk = PB_INSTALL((struct Library *)DOSBase, -198, thru, sizeof(struct pbhead), pbflags_stub);
    if (!blk) { put("patchflagstest: install: no block\npatchflagstest: FAIL\n"); return 20; }
    for (i = 0; i < 6; i++) {
        UBYTE c = ccrs[i];
        memset(out, 0, sizeof out);
        pbflags_call((struct Library *)DOSBase, c, out);
        line(names[i], out[20] == ((c & 1) ? 0xFF : 0) && out[21] == ((c & 2) ? 0xFF : 0) &&
                       out[22] == ((c & 4) ? 0xFF : 0) && out[23] == ((c & 8) ? 0xFF : 0));
        if (rd(out) != STUB_D0 || rd(out + 4) != STUB_D1 || rd(out + 8) != STUB_A0 ||
            rd(out + 12) != STUB_A1 || rd(out + 16) != (ULONG)DOSBase)
            regs_ok = 0;
    }
    line("regs", regs_ok);
    if (old) left = pb_remove(blk, (struct Library *)DOSBase, -198, pb_thruold_tmpl, pb_thruold_entry, 0);
    else left = PB_REMOVE(blk, (struct Library *)DOSBase, -198, thru, 0);
    line("removed", left == 0 && *(APTR *)((UBYTE *)DOSBase + -198 + 2) == vec0);
    put(bad ? "patchflagstest: FAIL\n" : "patchflagstest: OK\n");
    return bad ? 5 : 0;
}
