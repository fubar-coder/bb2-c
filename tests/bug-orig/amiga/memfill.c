/*
 * memfill - print what two fresh blocks of memory hold, for the self test
 * of inject's fill= option (tests/bug-orig/inject-fill.cmake):
 *
 *   plain: XX XX ... (16 bytes)        AllocMem(16, MEMF_PUBLIC)
 *   clear: XX XX ... (16 bytes)        AllocMem(16, MEMF_PUBLIC|MEMF_CLEAR)
 *   behind: XX XX XX XX                with the argument "behind": the 4
 *                                      bytes behind the plain block (only
 *                                      defined with inject pad: zeros)
 *
 * usage: memfill [behind]      (rc 0, rc 20 without memory)
 */
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

static void dump(const char *what, const UBYTE *p, int n)
{
    char line[80];
    int i, k;

    k = sprintf(line, "%s:", what);
    for (i = 0; i < n; i++)
        k += sprintf(line + k, " %02x", p[i]);
    strcpy(line + k, "\n");
    Write(Output(), line, strlen(line));
}

int main(int argc, char **argv)
{
    UBYTE *plain = AllocMem(16, MEMF_PUBLIC);
    UBYTE *clear = AllocMem(16, MEMF_PUBLIC | MEMF_CLEAR);
    int rc = 20;

    if (plain && clear) {
        dump("plain", plain, 16);
        dump("clear", clear, 16);
        if (argc > 1 && !strcmp(argv[1], "behind"))
            dump("behind", plain + 16, 4);
        rc = 0;
    }
    if (clear)
        FreeMem(clear, 16);
    if (plain)
        FreeMem(plain, 16);
    return rc;
}
