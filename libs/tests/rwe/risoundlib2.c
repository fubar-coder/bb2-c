/*
 * rwe risoundlib2 - the C build of the RWE userlib risoundlib2
 * (libs/userlibs/rwe/risoundlib2.c) or the reference build of its
 * source rwelibsrc/soundlib/risoundlib2.bb (risoundlib2.orig), one set as
 * LIBSET says, observed as cases (rwecmp.h; the expected
 * data libs/tests/expected/rwe_risoundlib2.txt).
 *
 *   risoundlib2 DIR     (DIR: risoundlib2 and memlib .obj/.orig)
 *
 * The commands talk to a device of this test named "ahi.device", in the
 * device list with priority 10 (OpenDevice finds it before the
 * ahi.device the emulator's Workbench has in DEVS:, whose requester
 * would wait for the user).  First every command with its OpenDevice
 * failing (OpenAHI: 0; the others return 0 or do nothing), then with it
 * opening: Open, Close, BeginIO, AbortIO and the library vectors of
 * ahi.device logged - AHI_AllocAudioRequestA (-120: the tag list
 * copied) and AHI_AudioRequestA (-126: the requester, its tags), the
 * others counted; it can fail OpenDevice, AHI_AllocAudioRequestA and
 * the requester (0 = cancelled).  Every command and form is called with
 * its inputs (other registers = markers); observed are d0, what the
 * device saw and the kept registers.
 * BUG(orig) IDs in brackets (libs/userlibs/rwe/risoundlib2.c).
 * Covered: init; every command without ahi.device; OpenAHI (both forms,
 * OpenDevice failing, again while open: opened again
 * (#924)); PlaySoundAHI and PlayRawAHI (all
 * forms: they never get to the device, same bug); AHIOutputRequest in
 * all six forms (the tag list it passes, also checked for its values:
 * (#931), the mixed-up entries of 1, 2
 * and 4 arguments with the markers in d2-d5
 * (#928), no AHI_FreeAudioRequest; the
 * allocation failing, the requester cancelled); AHIRequestedID and
 * AHIRequestedFreq (-1 and 0: (#934));
 * CloseAHI (no CloseDevice), CloseAHI while the request is in use
 * (CheckIO: nothing freed), finit, init again, init without a message
 * port (CreateMsgPort fails); CludgeSound (the rate at +16 of the object,
 * +14 not cleared: (#926),
 * (#925)).
 * Not covered: the real ahi.device (its requester); playing (the code
 * behind the ahidevice check is never reached); OpenAHI after CloseAHI
 * or without a request (writes to address 48, OpenDevice with request
 * 0: (#927)); finit without a request
 * (CheckIO of address 0); risoundlib's CludgeSound cases
 * (libs/tests/rwe/risoundlib.c, the same code).
 */
#include "rwecmp.h"
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/memory.h>

#define LIB "risoundlib2"

enum { CLUDGE, OPENAHI, CLOSEAHI, PLAYSOUND, PLAYRAW, OUTREQ, REQID, REQFREQ, FINIT, INIT };

/* ------------------------------------------------------ the test device */

static struct Device *dev;
static BOOL failopen;                   /* OpenDevice fails */
static BOOL allocfail;                  /* AHI_AllocAudioRequestA returns 0 */
static ULONG audioret = 1;              /* AHI_AudioRequestA's result */
static ULONG fakereq[16];               /* the "requester" */

static struct log {
    ULONG opens, closes, failed, beginios, aborts, allocs, requests, others;
    ULONG unit, flags;
    UWORD version;                      /* ahir_Version at OpenDevice */
    ULONG tags[17];                     /* AHI_AllocAudioRequestA's tag list */
    ULONG reqok, reqtags;               /* AHI_AudioRequestA: a0 = the
                                           requester, a1 */
} last;
static struct IORequest *opened;

static void dev_open(struct IORequest *ior __asm("a1"), LONG unit __asm("d0"),
                     ULONG flags __asm("d1"), struct Device *d __asm("a6"))
{
    struct log *l = &last;

    l->unit = unit;
    l->flags = flags;
    l->version = *(UWORD *)((UBYTE *)ior + 48);
    if (failopen) {
        ior->io_Error = IOERR_OPENFAIL;
        l->failed++;
        return;
    }
    ior->io_Error = 0;
    ior->io_Device = d;
    ior->io_Unit = (struct Unit *)unit;
    d->dd_Library.lib_OpenCnt++;
    l->opens++;
    opened = ior;
}

static ULONG dev_close(struct IORequest *ior __asm("a1"), struct Device *d __asm("a6"))
{
    (void)ior;
    d->dd_Library.lib_OpenCnt--;
    last.closes++;
    return 0;
}

static ULONG dev_null(void)
{
    return 0;
}

static void dev_beginio(struct IORequest *ior __asm("a1"))
{
    last.beginios++;
    ior->io_Error = 0;
    if (!(ior->io_Flags & IOF_QUICK))
        ior->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
}

static ULONG dev_abortio(void)
{
    last.aborts++;
    return 0;
}

static ULONG ahi_alloc(ULONG *tags __asm("a0"))
{
    struct log *l = &last;

    l->allocs++;
    memcpy(l->tags, tags, sizeof l->tags);
    return allocfail ? 0 : (ULONG)fakereq;
}

static ULONG ahi_request(ULONG *req __asm("a0"), ULONG tags __asm("a1"))
{
    struct log *l = &last;

    l->requests++;
    l->reqok = req == fakereq;
    l->reqtags = tags;
    return audioret;
}

static ULONG ahi_other(void)
{
    last.others++;
    return 0;
}

/* Open Close Expunge Null BeginIO AbortIO, then -42 .. -162 */
#define NVEC (6 + 21)
static APTR devfuncs[NVEC + 1];

static BOOL add_device(void)
{
    int i;

    devfuncs[0] = (APTR)dev_open;
    devfuncs[1] = (APTR)dev_close;
    devfuncs[2] = (APTR)dev_null;
    devfuncs[3] = (APTR)dev_null;
    devfuncs[4] = (APTR)dev_beginio;
    devfuncs[5] = (APTR)dev_abortio;
    for (i = 6; i < NVEC; i++)
        devfuncs[i] = (APTR)ahi_other;
    devfuncs[120 / 6 - 1] = (APTR)ahi_alloc;
    devfuncs[126 / 6 - 1] = (APTR)ahi_request;
    devfuncs[NVEC] = (APTR)-1;
    dev = (struct Device *)MakeLibrary((CONST_APTR)devfuncs, NULL, NULL, sizeof(struct Device),
                                       0);
    if (!dev)
        return FALSE;
    dev->dd_Library.lib_Node.ln_Type = NT_DEVICE;
    dev->dd_Library.lib_Node.ln_Name = (char *)"ahi.device";
    dev->dd_Library.lib_Node.ln_Pri = 10;  /* before a real one */
    dev->dd_Library.lib_Version = 6;
    AddDevice(dev);
    return TRUE;
}

static void rem_device(void)
{
    Forbid();
    Remove(&dev->dd_Library.lib_Node);
    Permit();
    FreeMem((UBYTE *)dev - dev->dd_Library.lib_NegSize,
            dev->dd_Library.lib_NegSize + dev->dd_Library.lib_PosSize);
}

static void same_device(void)
{
    OBS("opens", last.opens);
    OBS("closes", last.closes);
    OBS("failed opens", last.failed);
    OBS("BeginIOs", last.beginios);
    OBS("AbortIOs", last.aborts);
    OBS("AHI_AllocAudioRequestA", last.allocs);
    OBS("AHI_AudioRequestA", last.requests);
    OBS("other AHI calls", last.others);
    OBS("unit", last.unit);
    OBS("flags", last.flags);
    OBS("ahir_Version", last.version);
    OBS("requester", last.reqok);
    OBS("requester tags", last.reqtags);
    OBSBYTES("tag list", last.tags, sizeof last.tags);
}

/* ------------------------------------------------------------ the calls */

/* entry idx, form `form`, d0-d5 = the arguments (markers for the rest) */
static void call6(int idx, int form, const char *name, const ULONG *d, int n)
{
    ROUT = name;
    DO1(ENT(LIB, idx, form), {
        int i_;
        for (i_ = 0; i_ < n; i_++)
            R.d[i_] = d[i_];
    });
    OBS("d0", R.d[0]);
    same_device();
}

static void call(int idx, int form, const char *name)
{
    call6(idx, form, name, NULL, 0);
}

/* a statement: d0 is what the last call inside left there */
static void stmt(int idx, int form, const char *name, const ULONG *d, int n)
{
    ROUT = name;
    DONTCARE = 1;
    DO1(ENT(LIB, idx, form), {
        int i_;
        for (i_ = 0; i_ < n; i_++)
            R.d[i_] = d[i_];
    });
    DONTCARE = 0;
    same_device();
}

static UBYTE obj[48];                   /* a sound object at +8, guards */
#define OFF 8

static void playcmds(const char *what)
{
    static const ULONG raw[5] = { 0x1000, 200, 22050, 0x8000, 0x4000 };
    ULONG snd[3];
    int f;

    sprintf(VEC, "%s", what);
    for (f = 0; f < 3; f++) {
        ROUT = "PlaySoundAHI";
        DONTCARE = 1;
        snd[0] = 0;
        snd[1] = 0x20000;
        snd[2] = 0x1000;
        DO1(ENT(LIB, PLAYSOUND, f), {
            int i_;
            for (i_ = 0; i_ <= f; i_++)
                R.d[i_] = snd[i_];
            R.a[0] = (ULONG)(obj + OFF);
        });
        DONTCARE = 0;
        same_device();
        stmt(PLAYRAW, f, "PlayRawAHI", raw, 3 + f);
    }
}

static void reqcmds(const char *what)
{
    sprintf(VEC, "%s", what);
    call(REQID, 0, "AHIRequestedID");
    call(REQFREQ, 0, "AHIRequestedFreq");
}

/* AHIOutputRequest: the forms with 0, 1, 2, 4, 5, 6 arguments */
static BOOL expect;                     /* check the values as well */

static void outreqs(const char *what)
{
    static const int nargs[6] = { 0, 1, 2, 4, 5, 6 };
    static const ULONG a[6] = { 10, 20, 300, 200, 0x00020004, 44100 };
    int f;

    for (f = 0; f < 6; f++) {
        sprintf(VEC, "%s, %d arguments", what, nargs[f]);
        call6(OUTREQ, f, "AHIOutputRequest", a, nargs[f]);
        if (expect && f == 0)           /* the mode id 0 as the tag id */
            BOTH("0 arguments: tags", last.tags[1] == (ULONG)-1
                 && last.tags[4] == (ULONG)-1 && last.tags[6] == 0);
        if (expect && f == 3)           /* 4 arguments: x..height lost */
            BOTH("4 arguments: tags", last.tags[1] == (ULONG)-1
                 && last.tags[3] == (ULONG)-1 && last.tags[6] == MARK(4)
                 && last.tags[7] == 0);
        if (expect && f == 5)           /* width, mode id as tag ids */
            BOTH("6 arguments: tags", last.tags[1] == 10 && last.tags[4] == 300
                 && last.tags[5] == 200 && last.tags[6] == 0x00020004
                 && last.tags[7] == 44100 && last.tags[9] == (ULONG)-1
                 && last.tags[11] == 0 && last.tags[13] == 0);
        reqcmds(VEC);
        if (expect)                     /* the initial tags' data */
            BOTH("AHIRequestedFreq 0", R.d[0] == 0);
    }
}

static void openahi(int form, ULONG unit, ULONG id, const char *what)
{
    ULONG d[2] = { unit, id };

    sprintf(VEC, "%s unit %lu", what, unit);
    call6(OPENAHI, form, "OpenAHI", d, form + 1);
}

/* ----------------------------------------------------------- CludgeSound */

static UBYTE file[256];

static void putl(UBYTE *p, ULONG v)
{
    p[0] = (UBYTE)(v >> 24);
    p[1] = (UBYTE)(v >> 16);
    p[2] = (UBYTE)(v >> 8);
    p[3] = (UBYTE)v;
}

/* FORM 8SVX VHDR(20, rate) BODY(len) */
static void cludge(UWORD rate, ULONG len, const char *what)
{
    UBYTE *f = file;
    ULONG i;

    memset(f, 0, sizeof file);
    memcpy(f, "FORM", 4);
    memcpy(f + 8, "8SVX", 4);
    memcpy(f + 12, "VHDR", 4);
    putl(f + 16, 20);
    f[12 + 20] = (UBYTE)(rate >> 8);            /* the word at VHDR + 20 */
    f[12 + 21] = (UBYTE)rate;
    memcpy(f + 40, "BODY", 4);
    putl(f + 44, len);
    for (i = 0; i < len; i++)
        f[48 + i] = (UBYTE)(i * 3 + 1);
    putl(f + 4, 40 + len);
    sprintf(VEC, "%s", what);
    ROUT = "CludgeSound";
    DO1(ENT(LIB, CLUDGE, 0), R.d[0] = 1; R.d[1] = (ULONG)file;
        R.a[0] = (ULONG)(obj + OFF));
    OBS("d0", R.d[0]);
    OBSBYTES("object fields and behind", obj + OFF + 4, sizeof obj - OFF - 4);
    OBSBYTES("guards before", obj, OFF);
    OBS("+14 (cL)", *(UWORD *)(obj + OFF + 14));
    OBS("+16 (rate)", *(UWORD *)(obj + OFF + 16));
    if (R.d[0])
        BOTH("sample", memcmp(*(UBYTE **)(obj + OFF), file + 48, len) == 0);
}

/* ------------------------------------------------------------------ main */

static void memlib(int idx)
{
    markregs(&R);
    callstk(entry_form(getlib(&loaded, "memlib"), idx, 0), &R, NULL, 0, &POP);
}


static void lifecycle(int idx, const char *name, const char *what)
{
    ROUT = name;
    strcpy(VEC, what);
    DONTCARE = 1;
    DO1(ENT(LIB, idx, 0), (void)0);
    DONTCARE = 0;
    same_device();
}

int main(int argc, char **argv)
{
    static const char *names[] = { "memlib", LIB, NULL };
    ULONG avail;
    APTR old;

    if (argc < 2) {
        printf("usage: risoundlib2 DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("init", d2-d7|a2|a3);
    KEEPS("CloseAHI", d2-d7|a2|a3);
    KEEPS("AHIOutputRequest", d6|d7|a2|a3);
    KEEPS("AHIRequestedID", d1-d7|a0-a3|a6);
    KEEPS("AHIRequestedFreq", d1-d7|a0-a3|a6);
    KEEPS("PlaySoundAHI", d0|d3-d7|a0-a3|a6);
    KEEPS("PlayRawAHI", d1|d2|d5-d7|a0-a3|a6);
    KEEPS("OpenAHI", d2-d7|a2|a3);
    KEEPS("CludgeSound", d3-d7|a0|a2|a3|a6);
    KEEPS("finit", d2-d7|a2|a3);
    /* the test device before any OpenDevice: the Workbench of the
       emulator has an ahi.device in DEVS: (its requester would wait for
       the user) - this one has a higher priority, so OpenDevice always
       finds it first */
    if (!add_device()) {
        check("MakeLibrary ahi.device", FALSE);
        return 20;
    }
    memset(obj, 0xa5, sizeof obj);
    memset(obj + OFF, 0, 16);
    obj[OFF + 14] = 'c';
    obj[OFF + 15] = 'L';

    avail = AvailMem(MEMF_ANY);
    rwe_mem_start();
    ROUT = "memlib init";
    strcpy(VEC, "");
    memlib(7);

    lifecycle(INIT, "init", "");

    /* without ahi.device: OpenDevice fails */
    failopen = TRUE;
    strcpy(VEC, "no device");
    lifecycle(CLOSEAHI, "CloseAHI", "nothing open");
    lifecycle(INIT, "init", "again");
    outreqs("no device");
    reqcmds("no device");
    playcmds("no device");
    openahi(0, 0, 0, "no ahi.device");
    openahi(1, 1, 5, "no ahi.device");
    outreqs("no device opened");
    failopen = FALSE;

    /* CludgeSound: the rate at +16, +14 kept (BUG(orig)) */
    cludge(8363, 100, "8363 Hz");
    cludge(22050, 33, "22050 Hz");
    cludge(16000, 2, "16000 Hz, after it");

    /* the test device opens */
    openahi(0, 0, 0, "test device");
    BOTH("OpenAHI opened", R.d[0] == (ULONG)-1 && last.opens == 1);
    playcmds("open");                   /* nothing: BUG(orig) */
    BOTH("nothing played", last.beginios == 0);
    expect = TRUE;
    outreqs("open");
    expect = FALSE;
    call(REQID, 0, "AHIRequestedID");
    BOTH("AHIRequestedID -1", R.d[0] == (ULONG)-1);
    BOTH("six requesters", last.allocs == 6 && last.requests == 6);
    audioret = 0;
    outreqs("cancelled");
    audioret = 1;
    allocfail = TRUE;
    outreqs("no requester");
    allocfail = FALSE;
    reqcmds("open");
    /* OpenAHI again: opened again (BUG(orig)) */
    openahi(1, 2, 0x20004, "again,");
    playcmds("open twice");

    /* CloseAHI while the request is "in use" (CheckIO 0): nothing */
    opened->io_Flags = 0;
    opened->io_Message.mn_Node.ln_Type = NT_MESSAGE;
    lifecycle(CLOSEAHI, "CloseAHI", "request in use");
    reqcmds("request in use");
    opened->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    /* CloseAHI: no CloseDevice (BUG(orig)), port and request gone */
    lifecycle(CLOSEAHI, "CloseAHI", "open");
    reqcmds("closed");
    outreqs("closed");
    lifecycle(CLOSEAHI, "CloseAHI", "again");
    lifecycle(FINIT, "finit", "closed");

    /* init again, open, finit */
    lifecycle(INIT, "init", "after finit");
    openahi(1, 3, 0, "after init,");
    outreqs("reopened");
    opened->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    lifecycle(FINIT, "finit", "open");

    /* init without a message port: finit does nothing */
    old = SetFunction((struct Library *)SysBase, -666, (APTR)rwe_fail0);
    lifecycle(INIT, "init", "no port");
    SetFunction((struct Library *)SysBase, -666, old);
    reqcmds("no port");
    playcmds("no port");
    lifecycle(FINIT, "finit", "no port");
    lifecycle(INIT, "init", "last");
    lifecycle(FINIT, "finit", "last");

    ROUT = "memlib finit";
    memlib(8);
    rwe_mem_check(LIB, avail, TRUE);
    rem_device();                       /* made before the count */
    return rwe_done(LIB);
}
