/*
 * test_epcheck.c - the check of the __entrypoint routines (tools/lib/epcheck)
 * with objdump texts from memory: a routine that keeps a6 passes, one that
 * writes a6 without saving it fails, one that saves a6 at its start
 * passes; a call and a jump into a routine from another function fail
 * (a C call, identical code folding), a routine that begins with a jump
 * into another one fails; an archive with two members.
 */
#define _DEFAULT_SOURCE
#include "testutil.h"
#include "epcheck.h"

static char rep[4096];

static long run(const char *symtab, const char *disasm, long *n)
{
    FILE *f = fmemopen(rep, sizeof rep, "w");
    long r;

    memset(rep, 0, sizeof rep);
    r = epcheck_text(symtab, disasm, f, n);
    fclose(f);
    return r;
}

#define HEAD "In archive libx.a:\n\nx.c.obj:     file format amiga\n\n"

static const char symtab[] = HEAD
    "SYMBOL TABLE:\n"
    "00000000 g       .text      0000 01 _ep\n"
    "00000000 l       .text      0000 21 _ep_c\n"
    "00000000 g       .text      0000 01 ___blitz_entrypoint_ep\n"
    "00000010 l       .text      0000 21 _plain\n"
    "00000000         *UND*      0001 81 _g\n"
    "00000000 l       .blitzlib.head 0000 21 _bl_header_0\n";

int main(void)
{
    long n;
    char d[2048];

    /* a routine that uses d2-d6 and keeps a6: passes */
    snprintf(d, sizeof d, HEAD "\nDisassembly of section .text:\n\n"
             "00000000 <_ep>:\n"
             "   0:\tmovel d0,d6\n"
             "   2:\tmoveq #0,d2\n"
             "   4:\tjsr 0 <_g>\n"
             "   a:\tbras 2 <_ep+0x2>\n"
             "   c:\tmovel d2,d0\n"
             "   e:\trts\n"
             "\n00000010 <_plain>:\n"
             "  10:\tmoveml d2-d6/a2/a6,sp@-\n"
             "  14:\tmoveal 4,a6\n"
             "  18:\tmoveml sp@+,d2-d6/a2/a6\n"
             "  1c:\trts\n");
    CHECK(run(symtab, d, &n) == 0);
    CHECK(n == 1);
    CHECK(rep[0] == 0);

    /* writes a6 without saving it: fails */
    snprintf(d, sizeof d, HEAD "\nDisassembly of section .text:\n\n"
             "00000000 <_ep>:\n"
             "   0:\tmovel d0,d6\n"
             "   2:\tmoveal 4,a6\n"
             "   6:\tjsr a6@(-198)\n"
             "   a:\trts\n"
             "\n00000010 <_plain>:\n"
             "  10:\trts\n");
    CHECK(run(symtab, d, &n) == 1);
    CHECK(strstr(rep, "x.c.obj: ep: the __entrypoint routine writes a6 without saving it at "
                      "its start (2: moveal 4,a6)") != NULL);

    /* a6 in a register list restored, a6@+ and lea: each a write */
    snprintf(d, sizeof d, HEAD "\nDisassembly of section .text:\n\n"
             "00000000 <_ep>:\n"
             "   0:\tmovel d2,sp@-\n"
             "   2:\tmoveml sp@+,d2/a2-a6\n"
             "   6:\tmovel d0,a6@+\n"
             "   8:\tleal a0@(4),a6\n"
             "   c:\tmovel a6@(4),d0\n"
             "   e:\trts\n"
             "\n00000010 <_plain>:\n"
             "  10:\trts\n");
    CHECK(run(symtab, d, &n) == 3);

    /* a6 saved at the start: passes */
    snprintf(d, sizeof d, HEAD "\nDisassembly of section .text:\n\n"
             "00000000 <_ep>:\n"
             "   0:\tmoveml d2/a6,sp@-\n"
             "   4:\tmoveal 4,a6\n"
             "   8:\tmoveml sp@+,d2/a6\n"
             "   c:\trts\n"
             "\n00000010 <_plain>:\n"
             "  10:\trts\n");
    CHECK(run(symtab, d, &n) == 0);

    /* a call into the routine from C, and the jump of identical code
       folding (the plain function with the same body): both fail */
    snprintf(d, sizeof d, HEAD "\nDisassembly of section .text:\n\n"
             "00000000 <_ep>:\n"
             "   0:\tmovel d0,d6\n"
             "   2:\trts\n"
             "\n00000010 <_plain>:\n"
             "  10:\tjsr %%pc@(0 <_ep>)\n"
             "  14:\tbsrw 2 <_ep_c+0x2>\n"
             "  18:\tbras 0 <_ep>\n");
    CHECK(run(symtab, d, &n) == 3);
    CHECK(strstr(rep, "x.c.obj: 18: bras 0 <_ep>: a call or jump into the __entrypoint routine "
                      "ep, which keeps no d2-d7/a2 for a caller") != NULL);
    CHECK(strstr(rep, "x.c.obj: 10: jsr %pc@(0 <_ep>): a call or jump") != NULL);

    /* the routine folded into the plain function: fails */
    snprintf(d, sizeof d, HEAD "\nDisassembly of section .text:\n\n"
             "00000000 <_ep>:\n"
             "   0:\tbras 10 <_plain>\n"
             "\n00000010 <_plain>:\n"
             "  10:\tmoveml d2-d6,sp@-\n"
             "  14:\tmoveml sp@+,d2-d6\n"
             "  18:\trts\n");
    CHECK(run(symtab, d, &n) == 1);
    CHECK(strstr(rep, "x.c.obj: ep: the __entrypoint routine begins with a jump into another "
                      "routine (0: bras 10 <_plain>): identical code folding") != NULL);

    /* data at an odd offset (a text in the code) that objdump shows as a
       branch into the routine is no instruction */
    snprintf(d, sizeof d, HEAD "\nDisassembly of section .text:\n\n"
             "00000000 <_ep>:\n"
             "   0:\trts\n"
             "\n00000010 <_plain>:\n"
             "  13:\tbvss 2 <_ep+0x2>\n"
             "  15:\trts\n");
    CHECK(run(symtab, d, &n) == 0);

    /* a branch of the bit test kind is no branch; a jump inside the routine
       is fine */
    snprintf(d, sizeof d, HEAD "\nDisassembly of section .text:\n\n"
             "00000000 <_ep>:\n"
             "   0:\tbtst d1,0 <_ep>\n"
             "   4:\tbeqs 0 <_ep>\n"
             "   6:\trts\n"
             "\n00000010 <_plain>:\n"
             "  10:\tbset #1,0 <_ep>\n"
             "  14:\trts\n");
    CHECK(run(symtab, d, &n) == 0);

    /* two members: the routine of the second one is checked with its own
       symbols */
    {
        static const char st2[] = HEAD
            "SYMBOL TABLE:\n"
            "00000000 g       .text      0000 01 _f\n"
            "\ny.c.obj:     file format amiga\n\n"
            "SYMBOL TABLE:\n"
            "00000000 g       .text      0000 01 _ep2\n"
            "00000000 g       .text      0000 01 ___blitz_entrypoint_ep2\n";

        snprintf(d, sizeof d, HEAD "\nDisassembly of section .text:\n\n"
                 "00000000 <_f>:\n"
                 "   0:\tmoveal d0,a6\n"
                 "   2:\trts\n"
                 "\ny.c.obj:     file format amiga\n\n"
                 "\nDisassembly of section .text:\n\n"
                 "00000000 <_ep2>:\n"
                 "   0:\tmoveal d0,a6\n"
                 "   2:\trts\n");
        CHECK(run(st2, d, &n) == 1);
        CHECK(n == 1);
        CHECK(strstr(rep, "y.c.obj: ep2: the __entrypoint routine writes a6") != NULL);
    }

    /* a marker without code */
    CHECK(run(symtab, HEAD, &n) == 1);
    CHECK(strstr(rep, "x.c.obj: ep: no code of the __entrypoint routine found") != NULL);

    if (failures)
        fprintf(stderr, "%s", rep);
    return failures != 0;
}
