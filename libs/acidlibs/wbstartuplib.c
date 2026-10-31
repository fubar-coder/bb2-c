/*
 * wbstartuplib - system library 65533 ($BF00): program start from CLI or
 * Workbench and the argument list (used by NPrint Par$/NumPars etc.
 * through its init value, no compiler token of its own: $BF00 has no
 * code).
 *
 * C translation of bblibsrc/wbstartuplib ("aaron was here for bum7");
 * ref/wbstartuplib.obj matches the source.
 *   init (d0 = &argData, the init value):
 *     argData   0.w  $FFFF run from CLI, 0 from Workbench
 *               2.l  number of arguments
 *               6.l  first argument
 *     argument  0.l next, 4.l text, 8.w length, 10.l lock   (DATSIZE 14)
 *     d7 = "bLtZ" (run from the editor) or a CLI process: the CLI string
 *     (a0) is copied to cliString and split into arguments in place
 *     (spaces separate, "..." quotes, the end of each argument is
 *     overwritten with 0).  Workbench: wait for the WBStartup message,
 *     CurrentDir to the program's lock, arguments = the WBArgs after the
 *     program itself.
 *   finit: free the argument list, reply the WBStartup message (under
 *     Forbid, so Workbench cannot unload the program before it ends).
 * Everything is C.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <dos/dosextens.h>
#include <workbench/startup.h>
#define __NOLIBBASE__
#define DOS_BASE_NAME dosbase
#include <proto/dos.h>
#undef __NOLIBBASE__

/* entry points (thunks, defined below) */
void codeinit(void);
void codefinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !syslibheader{#wbstartuplib,$bf00,$bf01,init,1,finit,errors} */
BLITZ_SYSLIBHEADER7(LIB_wbstartuplib, 0xbf00, 0xbf01, init, BL_LONGRET, finit,
                    BLITZ_ERROR_AREA);
BLITZ_NULLSUB(0, 0, 0);                         /* $BF00 */
BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);
BLITZ_LIBFIN();

#define DATSIZE 14

struct arg {
    struct arg *next;                           /* 0.l */
    UBYTE *text;                                /* 4.l */
    UWORD length;                               /* 8.w */
    BPTR lock;                                  /* 10.l */
} BL_PACKED;
static_assert(sizeof(struct arg) == DATSIZE);

static struct {
    UWORD fromcli;                              /* 0.w Was it run from CLI? */
    ULONG numargs;                              /* 2.l */
    struct arg *first;                          /* 6.l */
} BL_PACKED argData;
static_assert(sizeof argData == 10);

static BPTR thisLock;                           /* lock for the next AddArg */
static struct arg *prevArg;
static struct Message *wbMsg;
static UWORD inQuote;
static UBYTE cliString[256];

/* AddArg's message in the normal code (fixed, #305: in the original it
   lies after `errors`, in the error check area); errMsg2 is unused (its
   check is commented out in the original) */
static const char err1[] = "Can't read args!";
static const char errMsg2[] BLITZ_ERRCHK = "Missing closing quote in argument string.";

/* AddArg - allocate a new argument (text, thisLock) at the end of the
   list.  Fixed (#305): without memory the original jumps to `err`
   (runtime error, in the error check area with its message: dangling
   without debugger); here the runtime error is raised in the normal code
   with its message and NULL returned, the caller stops. */
static struct arg *addarg(UBYTE *text)
{
    struct arg *a = AllocMem(DATSIZE, MEMF_PUBLIC | MEMF_CLEAR);

    if (!a) {
        BLITZ_RUNERR(err1);
        return NULL;
    }
    if (!argData.first)
        argData.first = a;              /* first argument */
    else
        prevArg->next = a;              /* link args together */
    prevArg = a;
    a->text = text;
    a->lock = thisLock;
    return a;
}

/* GetCLIArg - skip spaces; an argument runs until 0, LF or the end char
   (space, or " if it starts with a quote).  Returns the start, *len = its
   length (word), *pp = behind the character that ended it. */
static UBYTE *getcliarg(UBYTE **pp, UWORD *len)
{
    UBYTE *p = *pp, *start, end = ' ', c;
    UWORD n = 0;

    inQuote = 0;
    while ((c = *p++) != 0 && c == ' ')     /* SkipSpaces */
        ;
    p--;
    if (*p == '"') {
        end = '"';                      /* end char is now quote */
        p++;
        inQuote = 1;
    }
    start = p;
    while ((c = *p++) != 0 && c != 10 && c != end)
        n++;
    /* (the check for a missing closing quote - err2 - is commented out
       in the original) */
    *pp = p;
    *len = n;
    return start;
}

/**
 * @brief init - read the program's arguments (CLI or Workbench).
 *
 * Started from the editor (d7 = "bLtZ") or from a CLI: the CLI string is
 * copied to cliString and split into arguments in place (spaces separate,
 * "..." quotes).  From Workbench: waits for the WBStartup message,
 * CurrentDir to the program's lock, the arguments are the WBArgs after
 * the program itself.  Keeps d1-d7/a1-a6.
 *
 * Errors: no memory for an argument node raises "Can't read args!" (see
 * addarg, fixed (#305)).
 *
 * BUG(orig) (#306, #307): dos.library is never closed, the old current
 * directory is not restored.  BUG(orig) (#308): no length check of the CLI
 * string (cliString has 256 bytes); see also #309.
 *
 * @param argptr a0: the CLI argument string
 * @param magic d7: "bLtZ" when started from Blitz
 * @return d0: &argData (the init value: from CLI, number of arguments,
 *         first argument)
 */
static ULONG BLITZ_ROUTINE codeinit_c(UBYTE *argptr BL_REG(a0), ULONG magic BL_REG(d7))
{
    argData.numargs = 0;
    argData.first = NULL;
    wbMsg = NULL;

    if (magic != 0x624c745a) {          /* "bLtZ" */
        struct Process *pr = (struct Process *)FindTask(NULL);

        if (!pr->pr_CLI) {
            /* was run from WB */
            struct WBStartup *wb;
            struct WBArg *wa;
            struct Library *dosbase;
            LONG n;

            argData.fromcli = 0;
            WaitPort(&pr->pr_MsgPort);
            wbMsg = GetMsg(&pr->pr_MsgPort);
            if (!wbMsg)
                return (ULONG)&argData;
            wb = (struct WBStartup *)wbMsg;
            /* BUG(orig) (#306, #307): dos.library is never closed, the old current
               directory is not restored */
            dosbase = OldOpenLibrary((STRPTR)"dos.library");
            CurrentDir(wb->sm_ArgList->wa_Lock);
            n = wb->sm_NumArgs - 1;     /* forget the program itself */
            argData.numargs = n;
            if (n <= 0)
                return (ULONG)&argData;
            wa = wb->sm_ArgList;
            do {
                struct arg *a;
                UBYTE *s;
                ULONG len = 0;

                wa++;                   /* point past first one! */
                thisLock = wa->wa_Lock;
                a = addarg((UBYTE *)wa->wa_Name);
                if (!a)
                    break;
                for (s = (UBYTE *)wa->wa_Name; *s++; )   /* SizeString */
                    len++;
                a->length = len;
            } while (--n > 0);
            return (ULONG)&argData;
        }
    }

    /* was run from CLI: copy the CLI string to our buffer.
       BUG(orig) (#308): no length check (cliString has 256 bytes). */
    {
        UBYTE *d = cliString, *p;

        while ((*d++ = *argptr++) != 0)
            ;
        p = cliString;
        argData.numargs = 0;
        argData.fromcli = 0xffff;
        for (;;) {
            UWORD len;
            UBYTE *text = getcliarg(&p, &len);
            struct arg *a;

            /* a quoted argument may be empty */
            if ((WORD)len <= 0 && !inQuote)
                break;
            a = addarg(text);
            if (!a)
                break;
            argData.numargs++;
            a->length = len;
            /* the character that ended it (0, LF, space or quote) becomes
               the 0 of this argument.  If it was the 0 at the end of the
               string, the scan goes on behind it - harmless in a program
               (cliString is cleared and written once, the next byte is 0);
               only with old contents (host test) or after the overflow of
               a full buffer (no length check above) extra arguments (#309). */
            p[-1] = 0;
        }
    }
    return (ULONG)&argData;
}
BLITZ_THUNK(codeinit, codeinit_c, "d1/a1");

/**
 * @brief finit - free the arguments, reply the Workbench message.
 *
 * The WBStartup message is replied under Forbid (no Permit: the program
 * ends, Workbench cannot unload it before).  argData.first is not cleared
 * (not a bug: codeinit clears it at every start).  The original changes
 * d0/a1-a3/a6 only when there are arguments / a message, the thunk keeps
 * d0-d1/a0-a1.
 */
static void BLITZ_ROUTINE codefinit_c(void)
{
    struct arg *a = argData.first, *next;

    while (a) {
        next = a->next;
        FreeMem(a, DATSIZE);
        a = next;
    }
    if (wbMsg) {
        Forbid();                       /* no Permit: the program ends */
        ReplyMsg(wbMsg);
    }
}
BLITZ_THUNK(codefinit, codefinit_c, "d0-d1/a0-a1");
