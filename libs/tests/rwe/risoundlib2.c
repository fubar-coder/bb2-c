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
 * device list with priority 10 (the test machine's Workbench has no
 * ahi.device; one installed in DEVS: would come after it, so its
 * requester never waits for the user).  First every command with its OpenDevice
 * failing (OpenAHI: 0; the others return 0 or do nothing), then with it
 * opening: Open, Close, BeginIO, AbortIO and the library vectors of
 * ahi.device logged - AHI_AllocAudioRequestA (-120: the tag list
 * copied), AHI_AudioRequestA (-126: the requester, its tags; a confirmed
 * requester gets a choice in ahiam_AudioID/ahiam_MixFreq) and
 * AHI_FreeAudioRequest (-132), the others counted; it can fail
 * OpenDevice, AHI_AllocAudioRequestA and the requester (0 = cancelled).
 * BeginIO logs the request's fields (CMD_WRITE: data, length, type,
 * frequency, volume, position, ahir_Link, priority) and completes it at
 * once (quick, or replied to the port), or keeps it while "hold" is set
 * (a sound that plays on) until AbortIO; a completed request gets markers
 * in ahir_Link and the priority (AHI's autodoc: "the other fields are
 * trashed").  AHIOutputRequest gets a used screen object of screenslib in
 * a0 (its struct Screen a constant marker), or 0.  Every command and form
 * is called with its inputs (other registers = markers); observed are d0,
 * what the device saw, the replies in the library's port and the kept
 * registers.
 * The issue numbers in brackets: the original's bug, fixed in the port
 * (libs/userlibs/rwe/risoundlib2.c; [orig]/[fixed #N] pairs).
 * Covered: init; every command without ahi.device; OpenAHI (both forms,
 * OpenDevice failing, again while open: opened again in the original,
 * 0 in the port (#924)); PlaySoundAHI and PlayRawAHI (all forms; in the
 * original they never get to the device (#924); the request's fields,
 * the length in bytes (#932)); AHIOutputRequest in all six forms (the
 * tag list it passes, also checked for its values (#931), the entries
 * of 1, 2 and 4 arguments with the markers in d2-d5 (#928), the screen
 * (#929), AHI_FreeAudioRequest (#930); the allocation failing, the
 * requester cancelled); AHIRequestedID and AHIRequestedFreq (#934);
 * CloseAHI (CloseDevice in the port), CloseAHI while the request is in
 * use (CheckIO: nothing freed in the original), finit, init again, init
 * without a message port (CreateMsgPort fails); sounds that play on and
 * ones that end at once, CloseAHI and finit while one plays (#933),
 * AHIOutputRequest without a used screen (#929); CludgeSound (the rate
 * at +16 of the object in the original, not in the port (#926); +14 not
 * cleared in the original, cleared in the port (#925)).
 * Not covered: the real ahi.device (its requester); OpenAHI after
 * CloseAHI or without a request (the original writes to address 48 and
 * passes request 0 to OpenDevice (#927): the Blitz test
 * risoundlib2-openahi-after-closeahi); finit without a request (the
 * original's CheckIO of address 0).
 * Then the CludgeSound cases of risoundlib's test (risoundcludge.h, the
 * same routine): risoundlib's bugs, the rate written into the guard
 * behind the object (#926), the fields' +14 cleared by the port only
 * (#925).
 */
#include "rwecmp.h"
#include "risoundcludge.h"
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <devices/ahi.h>

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
    ULONG frees, freeok;                /* AHI_FreeAudioRequest (-132): a0
                                           the requester */
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

/* the last CMD_WRITE (or other command) BeginIO saw: the AHIRequest's
   fields as the library set them (the data as an address, written
   relative to the sample or the raw buffer by obs_write) */
static struct wlog {
    ULONG command, flags, data, length, offset, type, freq, volume, position, link;
    ULONG pri;
} w;
static BOOL hold;                       /* BeginIO keeps a request that is
                                           not quick (a sound that plays) */
static struct IORequest *pending;       /* the request it keeps */

/* "the other fields are trashed" (AHI's autodoc, CMD_WRITE): on
   completion ahir_Link and the priority get markers */
static void dev_trash(struct IORequest *ior)
{
    ((struct AHIRequest *)ior)->ahir_Link = (struct AHIRequest *)0xdead1111;
    ior->io_Message.mn_Node.ln_Pri = 0x55;
}

static void dev_beginio(struct IORequest *ior __asm("a1"))
{
    struct AHIRequest *a = (struct AHIRequest *)ior;

    last.beginios++;
    w.command = a->ahir_Std.io_Command;
    w.flags = a->ahir_Std.io_Flags;
    w.data = (ULONG)a->ahir_Std.io_Data;
    w.length = a->ahir_Std.io_Length;
    w.offset = a->ahir_Std.io_Offset;
    w.type = a->ahir_Type;
    w.freq = a->ahir_Frequency;
    w.volume = a->ahir_Volume;
    w.position = a->ahir_Position;
    w.link = (ULONG)a->ahir_Link;
    w.pri = (UBYTE)ior->io_Message.mn_Node.ln_Pri;
    ior->io_Error = 0;
    if (ior->io_Flags & IOF_QUICK) {    /* done at once (DoIO) */
        dev_trash(ior);
        return;
    }
    if (hold) {                         /* plays on */
        ior->io_Message.mn_Node.ln_Type = NT_MESSAGE;
        pending = ior;
        return;
    }
    dev_trash(ior);
    ReplyMsg(&ior->io_Message);
}

/* a request still in the device's hands (kept by hold, or marked "in use"
   by the scenario) is replied with IOERR_ABORTED */
static ULONG dev_abortio(struct IORequest *ior __asm("a1"))
{
    last.aborts++;
    if (ior && ior->io_Message.mn_Node.ln_Type == NT_MESSAGE) {
        if (ior == pending)
            pending = NULL;
        ior->io_Error = IOERR_ABORTED;
        dev_trash(ior);
        ReplyMsg(&ior->io_Message);
    }
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
    if (audioret) {                     /* the user's choice (ahiam_AudioID,
                                           ahiam_MixFreq) */
        fakereq[0] = 0x00230000 | l->requests;
        fakereq[1] = 20000 + l->requests;
    }
    return audioret;
}

static void ahi_free(ULONG *req __asm("a0"))
{
    last.frees++;
    last.freeok = req == fakereq;
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
    devfuncs[132 / 6 - 1] = (APTR)ahi_free;
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
    OBS("AHI_FreeAudioRequest", last.frees);
    OBS("freed requester", last.freeok);
}

/* the request of the last BeginIO; its data relative to the sample of
   the sound object (`sample`) or the raw buffer of the test program */
static void obs_write(ULONG sample)
{
    OBS("io_Command", w.command);
    OBS("io_Flags", w.flags);
    OBSPTR("io_Data", w.data, "sample", sample);
    OBS("io_Length", w.length);
    OBS("io_Offset", w.offset);
    OBS("ahir_Type", w.type);
    OBS("ahir_Frequency", w.freq);
    OBS("ahir_Volume", w.volume);
    OBS("ahir_Position", w.position);
    OBS("ahir_Link", w.link);
    OBS("ln_Pri", w.pri);
}

/* ------------------------------------------------------------ the calls */

/* the used screen object of screenslib (a0 of AHIOutputRequest, !libs
   #screenslib,#ua0): the struct Screen (a constant marker: the test
   device never reads it) and the DBufInfo; scrobj = 0: no used screen */
static ULONG fakescreen[2] = { 0x5c5c0001, 0 };
static ULONG *scrobj = fakescreen;

/* entry idx, form `form`, d0-d5 = the arguments (markers for the rest);
   AHIOutputRequest gets scrobj in a0 */
static void call6(int idx, int form, const char *name, const ULONG *d, int n)
{
    ROUT = name;
    DO1(ENT(LIB, idx, form), {
        int i_;
        for (i_ = 0; i_ < n; i_++)
            R.d[i_] = d[i_];
        if (idx == OUTREQ)
            R.a[0] = (ULONG)scrobj;
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

static UBYTE rawbuf[200];               /* PlayRawAHI's data */

/* PlaySoundAHI of the object at obj + OFF in form f */
static void playsound(int f)
{
    static const ULONG snd[3] = { 0, 0x20000, 0x1000 };

    ROUT = "PlaySoundAHI";
    DONTCARE = 1;
    DO1(ENT(LIB, PLAYSOUND, f), {
        int i_;
        for (i_ = 0; i_ <= f; i_++)
            R.d[i_] = snd[i_];
        R.a[0] = (ULONG)(obj + OFF);
    });
    DONTCARE = 0;
    same_device();
    obs_write(*(ULONG *)(obj + OFF));
}

/* PlayRawAHI of rawbuf in form f */
static void playraw(int f)
{
    ULONG raw[5] = { (ULONG)rawbuf, sizeof rawbuf, 22050, 0x8000, 0x4000 };

    stmt(PLAYRAW, f, "PlayRawAHI", raw, 3 + f);
    obs_write(*(ULONG *)(obj + OFF));
}

/* the replies waiting in the port of the request the device saw opened
   (call it only while the library's port exists) */
static void obs_port(void)
{
    struct MsgPort *mp = opened->io_Message.mn_ReplyPort;
    struct Node *n;
    ULONG count = 0;

    for (n = mp->mp_MsgList.lh_Head; n->ln_Succ; n = n->ln_Succ)
        count++;
    OBS("replies in the port", count);
}

static void playcmds(const char *what)
{
    int f;

    sprintf(VEC, "%s", what);
    for (f = 0; f < 3; f++) {
        playsound(f);
        playraw(f);
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
    KEEPS("CludgeSound", d3-d7|a2|a3|a6);
    KEEPS("finit", d2-d7|a2|a3);
    /* the test device before any OpenDevice: with a higher priority than
       an ahi.device a Workbench may have in DEVS: (whose requester would
       wait for the user; the test machine's has none), so OpenDevice
       always finds it first */
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

    /* CludgeSound: the rate at +16 in the original (#926), +14 cleared by the port (#925) */
    cludge(8363, 100, "8363 Hz");
    cludge(22050, 33, "22050 Hz");
    cludge(16000, 2, "16000 Hz, after it");
    cs_cases(LIB, CLUDGE);              /* risoundlib's cases */

    /* the test device opens */
    openahi(0, 0, 0, "test device");
    BOTH("OpenAHI opened", R.d[0] == (ULONG)-1 && last.opens == 1);
    playcmds("open");                   /* the original: nothing (#924) */
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
    /* OpenAHI again: opened again in the original, 0 in the port (#924) */
    openahi(1, 2, 0x20004, "again,");
    playcmds("open twice");

    /* CloseAHI while the request is "in use" (CheckIO 0): nothing */
    opened->io_Flags = 0;
    opened->io_Message.mn_Node.ln_Type = NT_MESSAGE;
    lifecycle(CLOSEAHI, "CloseAHI", "request in use");
    reqcmds("request in use");
    opened->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    /* CloseAHI: the original - no CloseDevice, port and request gone
       (#924, #927) */
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

    /* sounds that play on (the device keeps the request, #933), sounds
       that end at once (their replies in the port), CloseAHI and finit
       while one plays; AHIOutputRequest without a used screen (#929) */
    lifecycle(INIT, "init", "for playing");
    openahi(0, 0, 0, "for playing,");
    opened->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    hold = TRUE;
    strcpy(VEC, "playing, first");
    playsound(0);
    obs_port();
    strcpy(VEC, "playing, second");
    playsound(1);
    obs_port();
    strcpy(VEC, "playing, raw");
    playraw(0);
    obs_port();
    strcpy(VEC, "playing, third");
    playsound(2);
    obs_port();
    lifecycle(CLOSEAHI, "CloseAHI", "playing");
    hold = FALSE;
    strcpy(VEC, "after CloseAHI");
    playsound(0);
    lifecycle(FINIT, "finit", "after CloseAHI");
    lifecycle(INIT, "init", "after playing");
    openahi(0, 0, 0, "after playing,");
    opened->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    strcpy(VEC, "ended, first");
    playsound(0);
    obs_port();
    strcpy(VEC, "ended, second");
    playsound(0);
    obs_port();
    scrobj = NULL;
    outreqs("no screen");
    scrobj = fakescreen;
    hold = TRUE;
    strcpy(VEC, "playing at finit");
    playsound(0);
    obs_port();
    lifecycle(FINIT, "finit", "playing");
    hold = FALSE;
    pending = NULL;

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
