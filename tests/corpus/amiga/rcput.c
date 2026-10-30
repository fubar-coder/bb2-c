/*
 * rcput - append "rc=N" to a file, N = the return code of the command the
 * shell ran before (cli_ReturnCode, what `If WARN` tests).  For the
 * session scripts on Kickstart 1.x (corpus.h, machine=), whose AmigaDOS
 * has neither $RC nor `>>`:
 *
 *   CORPUS:run >CORPUS:out/NAME.run CORPUS:out/NAME
 *   CORPUS:rcput CORPUS:out/NAME.run
 *
 * usage: rcput FILE     (FILE created if missing; rc 20 if it cannot be
 *                        written, 0 otherwise)
 * Linked with libnix13 (-mcrt=nix13): runs on Kickstart 1.x.
 */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

static const char usage[] =
    "usage: rcput FILE\n"
    "  append rc=N to FILE, N = the return code of the previous command\n";

int main(int argc, char **argv)
{
    struct Process *me = (struct Process *)FindTask(0);
    struct CommandLineInterface *cli = BADDR(me->pr_CLI);
    LONG rc = cli ? cli->cli_ReturnCode : 0;
    char buf[24], *p = buf + sizeof buf;
    ULONG u = rc < 0 ? -(ULONG)rc : (ULONG)rc;
    BPTR fh;
    LONG ok;

    if (argc != 2 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
        Write(Output(), (APTR)usage, sizeof usage - 1);
        return argc == 2 ? 0 : 20;
    }
    *--p = '\n';
    do {
        *--p = '0' + u % 10;
        u /= 10;
    } while (u);
    if (rc < 0)
        *--p = '-';
    *--p = '=';
    *--p = 'c';
    *--p = 'r';
    if (!(fh = Open(argv[1], MODE_OLDFILE)) && !(fh = Open(argv[1], MODE_NEWFILE)))
        return 20;
    Seek(fh, 0, OFFSET_END);
    ok = Write(fh, p, buf + sizeof buf - p) == buf + sizeof buf - p;
    Close(fh);
    return ok ? 0 : 20;
}
