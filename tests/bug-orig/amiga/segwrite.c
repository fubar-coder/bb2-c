/*
 * segwrite - writes behind the end of its own first hunk, for the self
 * test of inject's guardseg option (tests/bug-orig/inject-guardseg.cmake):
 *
 *   segwrite N OFFSET      writes N zero bytes (0-16) from OFFSET bytes
 *                          behind the end of the memory of its first hunk
 *                          (the size LoadSeg stored in front of it), and
 *                          prints "segwrite: N bytes at +OFFSET"
 *
 * Only safe under inject guardseg, which puts 32 KB behind the hunk (a
 * write within them is reported); with N = 0 it writes nothing and runs
 * anywhere.  No C startup: __start is the first code of the hunk, so the
 * hunk's memory begins 8 bytes in front of it (size, next segment).
 * rc 0, 20 for bad arguments.
 */
__asm__("  .text\n  .globl __start\n__start:\n  pea __start(pc)\n  move.l d0,-(sp)\n"
        "  move.l a0,-(sp)\n  jsr _run\n  lea 12(sp),sp\n  rts\n");

#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;

static void put(const char *s)
{
    LONG n = 0;

    while (s[n])
        n++;
    Write(Output(), (APTR)s, n);
}

/* the decimal number at *s (blanks before it skipped); -1 if none */
static LONG number(const char **s)
{
    LONG v = -1;

    while (**s == ' ')
        (*s)++;
    while (**s >= '0' && **s <= '9') {
        v = (v < 0 ? 0 : v * 10) + (**s - '0');
        (*s)++;
    }
    return v;
}

/* v in decimal (no division: the 68000 has none for longs, no libgcc) */
static void decimal(char *b, ULONG v)
{
    static const ULONG pow10[] = { 1000000000, 100000000, 10000000, 1000000, 100000,
                                   10000, 1000, 100, 10, 1 };
    int i, any = 0;

    for (i = 0; i < 10; i++) {
        char d = '0';

        while (v >= pow10[i]) {
            v -= pow10[i];
            d++;
        }
        if (d != '0' || any || i == 9) {
            *b++ = d;
            any = 1;
        }
    }
    *b = 0;
}

LONG run(const char *args, LONG len, UBYTE *start)
{
    const char *s = args;
    LONG n, off;
    UBYTE *end;
    char b[12];

    (void)len;
    SysBase = *(struct ExecBase **)4;
    if (!(DOSBase = (struct DosLibrary *)OpenLibrary((CONST_STRPTR)"dos.library", 36)))
        return 20;
    n = number(&s);
    off = number(&s);
    if (n < 0 || n > 16 || off < 0) {
        put("usage: segwrite N OFFSET\n");
        CloseLibrary((struct Library *)DOSBase);
        return 20;
    }
    end = start - 8 + *(ULONG *)(start - 8);
    for (LONG i = 0; i < n; i++)
        end[off + i] = 0;
    put("segwrite: ");
    decimal(b, (ULONG)n);
    put(b);
    put(" bytes at +");
    decimal(b, (ULONG)off);
    put(b);
    put("\n");
    CloseLibrary((struct Library *)DOSBase);
    return 0;
}
