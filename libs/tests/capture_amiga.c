/*
 * capture_amiga - capture_env.h for libs_tests: dos.library files and
 * shell variables, the test program's hunks and stack (exec.library).
 */
#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <exec/types.h>

#include "capture_env.h"

void *cap_env_create(const char *path)
{
    return (void *)Open((STRPTR)path, MODE_NEWFILE);
}

void cap_env_mkdir(const char *path)
{
    BPTR l = CreateDir((STRPTR)path);
    if (l)
        UnLock(l);
}

int cap_env_write(void *f, const char *p, uint32_t n)
{
    return Write((BPTR)f, (APTR)p, (LONG)n) != (LONG)n;
}

void cap_env_close(void *f)
{
    Close((BPTR)f);
}

int cap_env_getvar(const char *name, char *buf, uint32_t size)
{
    LONG n = GetVar((STRPTR)name, (STRPTR)buf, (LONG)size, 0);
    return n < 0 ? -1 : (int)n;
}

/* the memory of the test program itself: its hunks and its stack.  A
   value there is an address of the program's data (a string, a buffer, a
   return address), which moves with every change of a test */
#define MAXHUNKS 16
static ULONG hunk[MAXHUNKS][2];
static int nhunks;

void cap_env_find_prog(void)
{
    struct Process *pr = (struct Process *)FindTask(NULL);
    struct CommandLineInterface *cli = BADDR(pr->pr_CLI);
    BPTR seg = cli ? cli->cli_Module : pr->pr_SegList;

    nhunks = 0;
    if (!cli && seg)                    /* pr_SegList: the 4th entry */
        seg = ((BPTR *)BADDR(seg))[3];
    for (; seg && nhunks < MAXHUNKS - 1; seg = *(BPTR *)BADDR(seg)) {
        ULONG *p = BADDR(seg);
        hunk[nhunks][0] = (ULONG)p;
        hunk[nhunks][1] = (ULONG)p + p[-1];
        nhunks++;
    }
    hunk[nhunks][0] = (ULONG)pr->pr_Task.tc_SPLower;
    hunk[nhunks][1] = (ULONG)pr->pr_Task.tc_SPUpper;
    nhunks++;
}

int cap_env_is_prog(uint32_t v)
{
    int i;
    for (i = 0; i < nhunks; i++)
        if (v >= hunk[i][0] && v < hunk[i][1])
            return 1;
    return 0;
}
