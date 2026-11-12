/*
 * rwe ridebuglib - AddVarTrace, DelVarTrace, VarTraceWindow,
 * DisAsmWindow, CopperTrace, ProcControl, MemoryWindow: the C build or
 * the CD original (OL RIDEBUGLIB.OBJ), one set as LIBSET says, observed as
 * cases (rwecmp.h, LIBS_ONE_SET; the expected data
 * libs/tests/expected/rwe_ridebuglib.txt).
 *
 *   ridebuglib DIR
 *
 * The work of every command is in its error check routine, which sends a
 * message to the debugger through debuglib's docall ($D509).  The test
 * replaces docall by a stub (sysload_stubs) that records the message (all
 * 260 bytes, so the bytes a command leaves from the previous one are
 * observed too) and calls the checks of every form in one sequence:
 * AddVarTrace with every type 0..7, output 0..2 and names
 * (also the empty one, BUG(orig) (#612): copies on to the next 0), DelVarTrace,
 * VarTraceWindow, DisAsmWindow (called from a code block in front of
 * which there is a TRAP #1 word), CopperTrace with 0, an address, an
 * object + offset (also negative, and a 0 copper list), ProcControl
 * toggle/on/off, MemoryWindow.  The routines themselves (RTS) and
 * init/finit for the kept registers.  Not covered: the real debugger.
 */
#define LIBS_ONE_SET
#include "rwecmp.h"

#define LIB "ridebuglib"
enum { E_ADD, E_DEL, E_VARWIN, E_DISASM, E_COPPER, E_PROC, E_MEM, E_FINIT, E_INIT };

static UBYTE msg[260];
static ULONG nmsg;

static void docall_stub(const UBYTE *m __asm("a0"))
{
    memcpy(msg, m, sizeof msg);
    nmsg++;
}

static const struct stub stubs[] = { { 0xd509, (APTR)docall_stub }, { 0, NULL } };

/* a2 frame of AddVarTrace: -2(a2) = the type word */
static UWORD typeframe[2];
/* names: the empty name followed by more */
static const char names_buf[] = "\0after the empty name\0";
static UWORD code[4];
static ULONG coplist[4];

#define SENT() do {                                                         \
        OBS("messages", nmsg);                                              \
        OBSBYTES("message", msg, sizeof msg);                               \
    } while (0)

/* call the check of entry e, form f */
#define CHK(e, f, setup) DO1(ERR(LIB, e, f), setup)

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    static const char *vnames[] = { "a", "counter.l", "x$" };
    int t, k;

    if (argc < 2) {
        printf("usage: ridebuglib DIR\n");
        return 20;
    }
    sysload_stubs = stubs;
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("init", d0-d7|a0-a3|a6);
    KEEPS("routine (RTS)", d0-d7|a0-a3|a6);
    KEEPS("AddVarTrace", d4-d7|a3|a6);
    KEEPS("DelVarTrace", d2-d7|a2|a3|a6);
    KEEPS("VarTraceWindow", d2-d7|a2|a3|a6);
    KEEPS("DisAsmWindow", d2-d7|a2|a3|a6);
    KEEPS("CopperTrace", d2-d7|a2|a3|a6);
    KEEPS("ProcControl", d2-d7|a2|a3|a6);
    KEEPS("MemoryWindow", d2-d7|a2|a3|a6);
    KEEPS("finit", d0-d7|a0-a3|a6);
    strcpy(VEC, "");

    ROUT = "init";
    DO1(ENT(LIB, E_INIT, 0), (void)0);
    ROUT = "routine (RTS)";
    for (k = 0; k < E_FINIT; k++) {
        sprintf(VEC, "entry %d", k);
        DO1(ENT(LIB, k, 0), (void)0);
        OBS("same address", ENT(LIB, k, 0) == ENT(LIB, E_INIT, 0));
    }

    ROUT = "AddVarTrace";
    for (t = 0; t < 8; t++)
        for (k = 0; k < 3; k++) {
            sprintf(VEC, "type %d name %s output %d", t, vnames[k], k);
            CHK(E_ADD, 0, R.d[0] = 0x12345678; R.d[1] = (ULONG)vnames[k];
                R.d[2] = 0x5a5a5a00UL | k; typeframe[0] = t;
                R.a[2] = (ULONG)&typeframe[1]);
            SENT();
        }
    strcpy(VEC, "empty name");
    CHK(E_ADD, 0, R.d[0] = 0; R.d[1] = (ULONG)names_buf; R.d[2] = 7;
        typeframe[0] = 3; R.a[2] = (ULONG)&typeframe[1]);
    SENT();

    ROUT = "DelVarTrace";
    for (k = 0; k < 3; k++) {
        sprintf(VEC, "name %s", vnames[k]);
        CHK(E_DEL, 0, R.d[0] = (ULONG)vnames[k]);
        SENT();
    }
    strcpy(VEC, "empty name");
    CHK(E_DEL, 0, R.d[0] = (ULONG)names_buf);
    SENT();

    ROUT = "VarTraceWindow";
    strcpy(VEC, "");
    CHK(E_VARWIN, 0, (void)0);
    SENT();

    /* DisAsmWindow: called by code[] = TRAP #1 (never run), NOP, JSR (a6),
       RTS - the check finds the TRAP #1 word */
    ROUT = "DisAsmWindow";
    code[0] = 0x4e41;
    code[1] = 0x4e71;
    code[2] = 0x4e96;
    code[3] = 0x4e75;
    CacheClearU();
    {
        UBYTE *fn = ERR(LIB, E_DISASM, 0);
        DO1((APTR)&code[1], R.a[6] = (ULONG)fn);
    }
    OBS("messages", nmsg);
    OBS("address = the TRAP", *(ULONG *)(msg + 4) - (ULONG)code);
    *(ULONG *)(msg + 4) = 0;
    OBSBYTES("message", msg, sizeof msg);

    ROUT = "CopperTrace";
    coplist[0] = 0xc0c0c0c0UL;
    coplist[1] = 0;
    coplist[2] = 0x00abcdefUL;
    coplist[3] = 0x11111111UL;
    strcpy(VEC, "address 0");
    CHK(E_COPPER, 0, R.d[0] = 0);
    SENT();
    strcpy(VEC, "address $76543210");
    CHK(E_COPPER, 0, R.d[0] = 0x76543210UL);
    SENT();
    {
        static const WORD offs[] = { 0, 4, 8, -4 };
        for (k = 0; k < 4; k++) {
            sprintf(VEC, "object, offset %d", offs[k]);
            CHK(E_COPPER, 1, R.d[0] = (ULONG)&coplist[k == 3 ? 3 : 0];
                R.d[1] = 0xffff0000UL | (UWORD)offs[k]);
            SENT();
        }
        strcpy(VEC, "object 0");
        CHK(E_COPPER, 1, R.d[0] = 0; R.d[1] = 4);
        SENT();
    }

    ROUT = "ProcControl";
    strcpy(VEC, "toggle");
    CHK(E_PROC, 0, (void)0);
    SENT();
    for (k = 0; k < 3; k++) {
        static const ULONG on[] = { 0xffffffffUL, 0, 0x101 };
        sprintf(VEC, "$%lX", on[k]);
        CHK(E_PROC, 1, R.d[0] = on[k]);
        SENT();
    }
    strcpy(VEC, "toggle again");
    CHK(E_PROC, 0, (void)0);
    SENT();

    ROUT = "MemoryWindow";
    for (k = 0; k < 3; k++) {
        sprintf(VEC, "window %d", k);
        CHK(E_MEM, 0, R.d[0] = 0x10000 + k; R.d[1] = 0xdff000UL + 2 * k);
        SENT();
    }
    ROUT = "AddVarTrace";
    strcpy(VEC, "after the others");
    CHK(E_ADD, 0, R.d[0] = 4; R.d[1] = (ULONG)"z"; R.d[2] = 0;
        typeframe[0] = 7; R.a[2] = (ULONG)&typeframe[1]);
    SENT();

    BOTH("44 messages sent", nmsg == 44);

    ROUT = "finit";
    strcpy(VEC, "");
    DO1(ENT(LIB, E_FINIT, 0), (void)0);
    return rwe_done(LIB);
}
