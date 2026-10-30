/*
 * rwe risoundlib2 - the C build of the RWE userlib risoundlib2
 * (libs/userlibs/rwe/risoundlib2.c) against the reference build of its
 * source rwelibsrc/soundlib/risoundlib2.bb (risoundlib2.orig), method of
 * rwecmp.h.
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
 * the requester (0 = cancelled).  Every command and form is called on
 * both sets with the same inputs (other registers = markers); compared
 * are d0, what the device saw and every register the original keeps.
 * BUG(orig) IDs in brackets (libs/userlibs/rwe/risoundlib2.c).
 * Covered: init; every command without ahi.device; OpenAHI (both forms,
 * OpenDevice failing, again while open: opened again
 * [risoundlib2-ahidevice-never-set]); PlaySoundAHI and PlayRawAHI (all
 * forms: they never get to the device, same bug); AHIOutputRequest in
 * all six forms (the tag list it passes, also checked for its values:
 * [risoundlib2-outputrequest-tag-offsets], the mixed-up entries of 1, 2
 * and 4 arguments with the markers in d2-d5
 * [risoundlib2-outputrequest-forms], no AHI_FreeAudioRequest; the
 * allocation failing, the requester cancelled); AHIRequestedID and
 * AHIRequestedFreq (-1 and 0: [risoundlib2-requested-initial-tags]);
 * CloseAHI (no CloseDevice), CloseAHI while the request is in use
 * (CheckIO: nothing freed), finit, init again, init without a message
 * port (CreateMsgPort fails); CludgeSound (the rate at +16 of the object,
 * +14 not cleared: [risoundlib2-cludgesound-rate-behind-object],
 * [risoundlib2-cludgesound-cl-mark-kept]).
 * Not covered: the real ahi.device (its requester); playing (the code
 * behind the ahidevice check is never reached); OpenAHI after CloseAHI
 * or without a request (writes to address 48, OpenDevice with request
 * 0: [risoundlib2-openahi-after-closeahi]); finit without a request
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
static ULONG fakereq[2][16];            /* the "requesters" */

static struct log {
    ULONG opens, closes, failed, beginios, aborts, allocs, requests, others;
    ULONG unit, flags;
    UWORD version;                      /* ahir_Version at OpenDevice */
    ULONG tags[17];                     /* AHI_AllocAudioRequestA's tag list */
    ULONG reqok, reqtags;               /* AHI_AudioRequestA: a0 = the
                                           requester, a1 */
} last[2];
static struct IORequest *opened[2];

static void dev_open(struct IORequest *ior __asm("a1"), LONG unit __asm("d0"),
                     ULONG flags __asm("d1"), struct Device *d __asm("a6"))
{
    struct log *l = &last[cur];

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
    opened[cur] = ior;
}

static ULONG dev_close(struct IORequest *ior __asm("a1"), struct Device *d __asm("a6"))
{
    (void)ior;
    d->dd_Library.lib_OpenCnt--;
    last[cur].closes++;
    return 0;
}

static ULONG dev_null(void)
{
    return 0;
}

static void dev_beginio(struct IORequest *ior __asm("a1"))
{
    last[cur].beginios++;
    ior->io_Error = 0;
    if (!(ior->io_Flags & IOF_QUICK))
        ior->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
}

static ULONG dev_abortio(void)
{
    last[cur].aborts++;
    return 0;
}

static ULONG ahi_alloc(ULONG *tags __asm("a0"))
{
    struct log *l = &last[cur];

    l->allocs++;
    memcpy(l->tags, tags, sizeof l->tags);
    return allocfail ? 0 : (ULONG)fakereq[cur];
}

static ULONG ahi_request(ULONG *req __asm("a0"), ULONG tags __asm("a1"))
{
    struct log *l = &last[cur];

    l->requests++;
    l->reqok = req == fakereq[cur];
    l->reqtags = tags;
    return audioret;
}

static ULONG ahi_other(void)
{
    last[cur].others++;
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
    SAME("opens", last[cur].opens);
    SAME("closes", last[cur].closes);
    SAME("failed opens", last[cur].failed);
    SAME("BeginIOs", last[cur].beginios);
    SAME("AbortIOs", last[cur].aborts);
    SAME("AHI_AllocAudioRequestA", last[cur].allocs);
    SAME("AHI_AudioRequestA", last[cur].requests);
    SAME("other AHI calls", last[cur].others);
    SAME("unit", last[cur].unit);
    SAME("flags", last[cur].flags);
    SAME("ahir_Version", last[cur].version);
    SAME("requester", last[cur].reqok);
    SAME("requester tags", last[cur].reqtags);
    {
        const ULONG *t[2] = { last[0].tags, last[1].tags };
        SAMEBYTES("tag list", t, sizeof last[0].tags);
    }
}

/* ------------------------------------------------------------ the calls */

/* entry idx, form `form`, d0-d5 = the arguments (markers for the rest) */
static void call6(int idx, int form, const char *name, const ULONG *d, int n)
{
    ROUT = name;
    DO2(ENT(LIB, idx, form), {
        int i_;
        for (i_ = 0; i_ < n; i_++)
            R[cur].d[i_] = d[i_];
    });
    SAME("d0", R[cur].d[0]);
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
    DO2(ENT(LIB, idx, form), {
        int i_;
        for (i_ = 0; i_ < n; i_++)
            R[cur].d[i_] = d[i_];
    });
    DONTCARE = 0;
    same_device();
}

static UBYTE obj[2][48];                /* a sound object at +8, guards */
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
        DO2(ENT(LIB, PLAYSOUND, f), {
            int i_;
            for (i_ = 0; i_ <= f; i_++)
                R[cur].d[i_] = snd[i_];
            R[cur].a[0] = (ULONG)(obj[cur] + OFF);
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
            BOTH("0 arguments: tags", last[cur].tags[1] == (ULONG)-1
                 && last[cur].tags[4] == (ULONG)-1 && last[cur].tags[6] == 0);
        if (expect && f == 3)           /* 4 arguments: x..height lost */
            BOTH("4 arguments: tags", last[cur].tags[1] == (ULONG)-1
                 && last[cur].tags[3] == (ULONG)-1 && last[cur].tags[6] == MARK(4)
                 && last[cur].tags[7] == 0);
        if (expect && f == 5)           /* width, mode id as tag ids */
            BOTH("6 arguments: tags", last[cur].tags[1] == 10 && last[cur].tags[4] == 300
                 && last[cur].tags[5] == 200 && last[cur].tags[6] == 0x00020004
                 && last[cur].tags[7] == 44100 && last[cur].tags[9] == (ULONG)-1
                 && last[cur].tags[11] == 0 && last[cur].tags[13] == 0);
        reqcmds(VEC);
        if (expect)                     /* the initial tags' data */
            BOTH("AHIRequestedFreq 0", R[cur].d[0] == 0);
    }
}

static void openahi(int form, ULONG unit, ULONG id, const char *what)
{
    ULONG d[2] = { unit, id };

    sprintf(VEC, "%s unit %lu", what, unit);
    call6(OPENAHI, form, "OpenAHI", d, form + 1);
}

/* ----------------------------------------------------------- CludgeSound */

static UBYTE file[2][256];

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
    int s;
    ULONG i;

    for (s = 0; s < 2; s++) {
        UBYTE *f = file[s];
        memset(f, 0, sizeof file[s]);
        memcpy(f, "FORM", 4);
        memcpy(f + 8, "8SVX", 4);
        memcpy(f + 12, "VHDR", 4);
        putl(f + 16, 20);
        f[12 + 20] = (UBYTE)(rate >> 8);        /* the word at VHDR + 20 */
        f[12 + 21] = (UBYTE)rate;
        memcpy(f + 40, "BODY", 4);
        putl(f + 44, len);
        for (i = 0; i < len; i++)
            f[48 + i] = (UBYTE)(i * 3 + 1);
        putl(f + 4, 40 + len);
    }
    sprintf(VEC, "%s", what);
    ROUT = "CludgeSound";
    DO2(ENT(LIB, CLUDGE, 0), R[cur].d[0] = 1; R[cur].d[1] = (ULONG)file[cur];
        R[cur].a[0] = (ULONG)(obj[cur] + OFF));
    SAME("d0", R[cur].d[0]);
    {
        const UBYTE *fl[2] = { obj[0] + OFF + 4, obj[1] + OFF + 4 };
        const UBYTE *gb[2] = { obj[0], obj[1] };
        SAMEBYTES("object fields and behind", fl, sizeof obj[0] - OFF - 4);
        SAMEBYTES("guards before", gb, OFF);
    }
    SAME("+14 (cL)", *(UWORD *)(obj[cur] + OFF + 14));
    SAME("+16 (rate)", *(UWORD *)(obj[cur] + OFF + 16));
    if (R[0].d[0])
        BOTH("sample", memcmp(*(UBYTE **)(obj[cur] + OFF), file[cur] + 48, len) == 0);
}

/* ------------------------------------------------------------------ main */

static void memlib(int idx)
{
    for (cur = 0; cur < 2; cur++) {
        markregs(&R[cur]);
        callstk(entry_form(getlib(&sets[cur], "memlib"), idx, 0), &R[cur], NULL, 0, &POP[cur]);
    }
}

static void lifecycle(int idx, const char *name, const char *what)
{
    ROUT = name;
    strcpy(VEC, what);
    DONTCARE = 1;
    DO2(ENT(LIB, idx, 0), (void)0);
    DONTCARE = 0;
    same_device();
}

int main(int argc, char **argv)
{
    static const char *names[] = { "memlib", LIB, NULL };
    ULONG avail;
    APTR old;
    int s;

    if (argc < 2) {
        printf("usage: risoundlib2 DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    /* the test device before any OpenDevice: the Workbench of the
       emulator has an ahi.device in DEVS: (its requester would wait for
       the user) - this one has a higher priority, so OpenDevice always
       finds it first */
    if (!add_device()) {
        check("MakeLibrary ahi.device", FALSE);
        return 20;
    }
    memset(obj, 0xa5, sizeof obj);
    for (s = 0; s < 2; s++) {
        memset(obj[s] + OFF, 0, 16);
        obj[s][OFF + 14] = 'c';
        obj[s][OFF + 15] = 'L';
    }

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
    BOTH("OpenAHI opened", R[cur].d[0] == (ULONG)-1 && last[cur].opens == 1);
    playcmds("open");                   /* nothing: BUG(orig) */
    BOTH("nothing played", last[cur].beginios == 0);
    expect = TRUE;
    outreqs("open");
    expect = FALSE;
    call(REQID, 0, "AHIRequestedID");
    BOTH("AHIRequestedID -1", R[cur].d[0] == (ULONG)-1);
    BOTH("six requesters", last[cur].allocs == 6 && last[cur].requests == 6);
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
    for (s = 0; s < 2; s++) {
        opened[s]->io_Flags = 0;
        opened[s]->io_Message.mn_Node.ln_Type = NT_MESSAGE;
    }
    lifecycle(CLOSEAHI, "CloseAHI", "request in use");
    reqcmds("request in use");
    for (s = 0; s < 2; s++)
        opened[s]->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
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
    for (s = 0; s < 2; s++)
        opened[s]->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
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
