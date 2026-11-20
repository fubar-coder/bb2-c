/*
 * risoundlib2 - RWE userlib 45 (Reflective Images), the successor of
 * risoundlib with the same number: CludgeSound and sound output through
 * AHI (ahi.device): OpenAHI CloseAHI PlaySoundAHI PlayRawAHI
 * AHIOutputRequest AHIRequestedID AHIRequestedFreq.
 *
 * C translation of rwelibsrc/soundlib/risoundlib2.bb (BB22 format,
 * tools/bb22); no binary exists, the reference is its reference build
 * (blitz2 2.10, with the resident AHI.res made from the AHI SDK,
 * libs/userlibs/rwe/residents.cmake).  Number 45 = risoundlib's: built in
 * the variant rwe-1999-ahi, where it replaces risoundlib
 * (RWE_OWN_VARIANTS).  Notes: docs/rwelibs/port/risoundlib2.md.
 *
 * init:  CreateMsgPort, CreateIORequest (80 bytes, an AHIRequest), from
 *        exec V36 on
 * finit: nothing without the port or while CheckIO says the request is
 *        in use; else CloseAHI, DeleteIORequest, DeleteMsgPort
 * CloseAHI: CloseDevice if ahidevice is set, ahibase = 0; port and
 *        request stay
 * OpenAHI unit[,deviceID]: OpenDevice("ahi.device", unit, request, 0)
 *        with ahir_Version 4; -1 and ahibase = io_Device, or 0
 * PlaySoundAHI sound[,volume[,balance]] (a0 = audiolib's sound object):
 *        CMD_WRITE of the sample through SendIO; PlayRawAHI
 *        address,length,frequency[,volume[,balance]] through DoIO
 * AHIOutputRequest [x,y,width,height[,modeid[,frequency]]]: the tags of
 *        the requester changed, AHI_AllocAudioRequestA,
 *        AHI_AudioRequestA; its result (0 when cancelled)
 * AHIRequestedID, AHIRequestedFreq: two of those tags' data
 * CludgeSound sound#,address: risoundlib's (the original also stored the
 *        sample rate at +16 of the object, #926)
 *
 * All C: exec and ahi.device's library vectors through BLITZ_BASEJSR
 * where the original passes the whole d0 on, memlib through
 * BLITZ_ALIBJSR.  The tag list is the original's (in its code) with the
 * same offsets.
 *
 * BUG(orig):
 *   - AHIOutputRequest: the forms with 1, 2 and 4 arguments go to the
 *     wrong entries (4 arguments: x, y, width, height replaced by -1, the
 *     mode id whatever d4 holds; 1 argument: lost, d4/d5 whatever they
 *     hold; 2: d2/d3 whatever they hold) (#928);
 *     !libs's screen (a0) is
 *     not used, AHIR_Screen stays 0 (#929);
 *     every call allocates a requester that is never freed
 *     (#930).
 *   - AHIRequestedID and AHIRequestedFreq return the data of the initial
 *     tags (-1 and 0, never written), not the requester's selection
 *     (ahiam_AudioID, ahiam_MixFreq) (#934).
 *
 * Fixed (intended deviation, docs/libs/deviations.md 3a): CludgeSound
 * does not free the old sample when audiolib's "cL" mark (+14) is set
 * (DecodeSound's in-place data of chip memory, which memlib never
 * allocated; the original passed it to globfree: "Unable to Free
 * Memory") (#945).  CludgeSound clears +14 (audiolib's "cL" mark) again,
 * as risoundlib does; the original no longer did (its MOVE.w #0,14(a0)
 * became the write of the rate at +16), so after a DecodeSound of chip
 * data audiolib never freed CludgeSound's copy (#925).  As in risoundlib:
 * the sample rate is samplesPerSec, the word at offset 12 of the VHDR
 * data, not the word at VHDR + the chunk length (#832); a rate of 0
 * gives the result 0 instead of a period loop that never ends (#830); the
 * chunk walk skips the IFF pad byte behind a chunk of odd length and
 * stays inside the FORM, so a file without pad bytes no longer loads
 * (#828); the old sample is freed after the checks of the file and the
 * object's pointer cleared with it, a failed call leaves the sound as it
 * was (the original left the freed pointer, #831); the whole BODY is
 * copied with length / 2 words, an empty one or one of 131072 bytes or
 * more gives 0 (the original copied ((length - 1) & $FFFF) + 1 bytes,
 * #829).  CloseAHI closes the device and keeps the request and the
 * port for finit; OpenAHI without a request returns 0.  The original's
 * CloseAHI was finit: a later OpenAHI wrote ahir_Version to address 48
 * and passed request 0 to OpenDevice, the same without a request from
 * init (#927).  init makes port and request from exec V36 on only;
 * below (Kickstart 1.x) the commands fail.  The original called the V36
 * functions there, behind exec's jump table: every program with the
 * library crashed at its start (#1139).  OpenAHI sets ahidevice: the
 * original never did, so PlaySoundAHI and PlayRawAHI never played,
 * CloseAHI and finit never closed the device, and a second OpenAHI
 * opened the open request again (now 0); every write sets ahir_Link and
 * the priority, which the device trashes (#924).  The sound PlaySoundAHI
 * sent is taken back (aborted if it still plays, WaitIO, which removes
 * its reply from the port) before the next write, at CloseAHI and at
 * finit; the original sent its one request again while the device had
 * it, left the replies in the port and left everything open at finit
 * while the request was in use (#933).  PlaySoundAHI's io_Length is the
 * length in bytes, twice the object's length in words; the original gave
 * the words as bytes, half the sample (#932).  AHIOutputRequest writes
 * width, height, mode id and frequency into the data of their tags; the
 * original wrote them at the offsets 16, 20, 24, 28 of the tag list,
 * over the tag ids of width and height, so that width and mode id became
 * tag ids (mode id 0 = TAG_DONE ended the list) and the initial audio id
 * and frequency were never set (#931).  CludgeSound writes nothing behind audiolib's 16 byte sound
 * object; the original stored the rate at +16, the high word of the next
 * object's sample pointer (behind the last object whatever follows), for
 * PlaySoundAHI, which now takes its frequency from the object's period:
 * 3579545 / period (#926).
 */
#include "blitzlib.h"
#include "blitzobj.h"
#include "blitzexec.h"
#include <exec/io.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <devices/ahi.h>

/* entry points (thunks, below) */
void risoundlib2_loadsounds(void), risoundlib2_libinit(void);
void risoundlib2_libfinit(void), _OpenAHI(void), _OpenAHI2(void);
void _CloseAHI(void), _PlaySoundAHI(void), _PlaySoundAHI1(void);
void _PlaySoundAHI2(void), _PlayRawAHI(void), _PlayRawAHI1(void);
void _PlayRawAHI2(void), _AHIOutputRequest1(void), _AHIOutputRequest2(void);
void _AHIOutputRequest3(void), _AHIOutputRequest4(void);
void _AHIOutputRequest5(void), _AHIOutputRequest6(void), _AHIRequestedID(void);
void _AHIRequestedFreq(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#risoundlib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_risoundlib, init, BL_NORET, finit, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_audiolib, BL_IA0|BL_PD0, LIB_execlib, BL_LA6);
BLITZ_SUBS(risoundlib2_loadsounds, 0, 0);
BLITZ_NAME("CludgeSound", "sound#,address");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_OpenAHI), 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_OpenAHI2), 0, 0);
BLITZ_NAME("OpenAHI", "unit,[deviceID]");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_CloseAHI), 0, 0);
BLITZ_NAME("CloseAHI", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(LIB_audiolib, BL_IA0|BL_PD0);
BLITZ_SUBS(_PlaySoundAHI, 0, 0);
BLITZ_ARGS(BL_LONG, BL_QUICK);
BLITZ_LIBS(LIB_audiolib, BL_IA0|BL_PD0);
BLITZ_SUBS(_PlaySoundAHI1, 0, 0);
BLITZ_ARGS(BL_LONG, BL_QUICK, BL_QUICK);
BLITZ_LIBS(LIB_audiolib, BL_IA0|BL_PD0);
BLITZ_SUBS(_PlaySoundAHI2, 0, 0);
BLITZ_NAME("PlaySoundAHI", "sound,[volume,[balance]]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_PlayRawAHI, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_QUICK);
BLITZ_LIBS0();
BLITZ_SUBS(_PlayRawAHI1, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_QUICK, BL_QUICK);
BLITZ_LIBS0();
BLITZ_SUBS(_PlayRawAHI2, 0, 0);
BLITZ_NAME("PlayRawAHI", "address,length,frequency,[volume,[balance]]");

/* BUG(orig) (#928): the entries of 1, 2 and 4
   arguments are mixed up */
BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_screenslib, BL_UA0);
BLITZ_SUBS(BLITZ_AMIGAONLY(_AHIOutputRequest1), 0, 0);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(LIB_screenslib, BL_UA0);
BLITZ_SUBS(BLITZ_AMIGAONLY(_AHIOutputRequest3), 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_screenslib, BL_UA0);
BLITZ_SUBS(BLITZ_AMIGAONLY(_AHIOutputRequest4), 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_screenslib, BL_UA0);
BLITZ_SUBS(BLITZ_AMIGAONLY(_AHIOutputRequest2), 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_screenslib, BL_UA0);
BLITZ_SUBS(BLITZ_AMIGAONLY(_AHIOutputRequest5), 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_screenslib, BL_UA0);
BLITZ_SUBS(BLITZ_AMIGAONLY(_AHIOutputRequest6), 0, 0);
BLITZ_NAME("AHIOutputRequest", "[x,y,width,height[,modeid,[frequency]]]");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_AHIRequestedID, 0, 0);
BLITZ_NAME("AHIRequestedID", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_AHIRequestedFreq, 0, 0);
BLITZ_NAME("AHIRequestedFreq", "");

BLITZ_NULLSUB_AT(finit, risoundlib2_libfinit, 0, 0, LIB_doslib, BL_LA6);
BLITZ_NULLSUB_AT(init, risoundlib2_libinit, 0, 0);

BLITZ_LIBFIN();

#define ID(a, b, c, d) ((ULONG)(a) << 24 | (ULONG)(b) << 16 | (c) << 8 | (d))
#define LONGAT(p) (*(ULONG *)(p))
#define WORDAT(p) (*(UWORD *)(p))

/* the longest BODY: the length in words (+6 of the object) is a word, as
   audiolib's limit for LoadSound and DecodeSound ("Sound too long") */
#define MAXBODY 131071

/* audiolib's sound object (16 bytes): struct bb_sound (blitzobj.h) */

/**
 * @brief The playing frequency of a sound object: 3579545 / its period (+4).
 *
 * Fixed (#926): the original read the word +16 that CludgeSound wrote
 * behind the object; the period works for every sound object (LoadSound,
 * DecodeSound, InitSound, CludgeSound) and gives the pitch Sound plays.
 * Two DIVU.w steps (a library cannot link the division of libgcc).
 * @param period  the object's period
 * @return the frequency in Hz, 0 for a period of 0
 */
static ULONG periodfreq(UWORD period)
{
    ULONG hi, lo;

    if (!period)
        return 0;
    hi = bl_divu_w(3579545UL >> 16, period);               /* rem<<16 | quot */
    lo = bl_divu_w((hi & 0xffff0000UL) | (3579545UL & 0xffff), period);
    return (hi << 16) | (lo & 0xffff);
}

/* exec and ahi.device (AHI.res, lvo/ahi.i) */
#define LVO_OpenDevice              (-444)
#define LVO_DoIO                    (-456)
#define LVO_SendIO                  (-462)
#define LVO_AHI_AllocAudioRequestA  (-120)
#define LVO_AHI_AudioRequestA       (-126)

/* the source's constants (#MIN_AHI_VERSION) and AHI.res's; the request
   is a struct AHIRequest (#ahir_... = its offsets, AHIRequest_SIZEOF = 80)
   and AHIST_M8S, AHI_DEFAULT_ID come from devices/ahi.h */
#define MIN_AHI_VERSION     4
static_assert(sizeof(struct AHIRequest) == 80);
#define AHI_TAGBASE         0x80000000UL
#define AHIR_SCREEN         (AHI_TAGBASE + 201)
#define AHIR_INITIALLEFTEDGE (AHI_TAGBASE + 240)
#define AHIR_INITIALTOPEDGE (AHI_TAGBASE + 241)
#define AHIR_INITIALWIDTH   (AHI_TAGBASE + 242)
#define AHIR_INITIALHEIGHT  (AHI_TAGBASE + 243)
#define AHIR_INITIALAUDIOID (AHI_TAGBASE + 244)
#define AHIR_INITIALMIXFREQ (AHI_TAGBASE + 245)
#define AHIR_DOMIXFREQ      (AHI_TAGBASE + 260)

/* ahireqtags: the requester's tags (the original's labels ahileft,
   ahitop, ahiwidth, ahiheight, ahiaudioid, ahifrequency, ahiscreen, then
   DoMixFreq and TAG_END); AHIOutputRequest writes the longs at 4, 12, 16,
   20, 24, 28 - two of them tag ids; the port writes the data longs 4,
   12, 20, 28, 36, 44 (#931) */
static struct {
    struct TagItem left, top, width, height, audioid, mixfreq, screen, domixfreq;
    ULONG end;
} ahireqtags = {
    { AHIR_INITIALLEFTEDGE, (ULONG)-1 }, { AHIR_INITIALTOPEDGE, (ULONG)-1 },
    { AHIR_INITIALWIDTH, (ULONG)-1 }, { AHIR_INITIALHEIGHT, (ULONG)-1 },
    { AHIR_INITIALAUDIOID, (ULONG)-1 }, { AHIR_INITIALMIXFREQ, 0 },
    { AHIR_SCREEN, 0 }, { AHIR_DOMIXFREQ, 1 }, TAG_END
};
static_assert(sizeof ahireqtags == 17 * 4);
static APTR ahirequest;

static ULONG deviceunit, deviceid;
static struct MsgPort *ahimsgport;
static struct AHIRequest *ahiiorequest;
static ULONG ahidevice;                 /* ahi.device open (fixed: #924) */
static BOOL ahisent;                    /* a SendIO of PlaySoundAHI not taken
                                           back yet (fixed: #933) */
static struct Device *ahibase;
static char ahiname[] = "ahi.device";

/* the whole d0 of an exec call with the request in a1 */
#define reqcall(lvo, req) ({                                                \
    struct bl_regs r_ = { .a1 = (ULONG)(req) };                             \
    BLITZ_BASEJSR(SysBase, lvo, r_); })

/**
 * @brief init: create the message port and the AHI request.
 *
 * CreateMsgPort, then CreateIORequest of a struct AHIRequest (80 bytes)
 * on that port.  Both are exec V36 calls: below exec V36 (Kickstart 1.x)
 * port and request stay 0, and the commands fail (OpenAHI 0); fixed
 * (#1139), the original called them there (offsets behind exec's jump
 * table: a crash at the program's start).  Port and request made before
 * and not deleted by finit (CloseAHI keeps them, fixed: #927) stay.
 * @return d0: the request, 0 if the port or the request could not be
 *         made (a port made stays) or exec is older than V36
 */
static ULONG BLITZ_ROUTINE c_libinit(void)
{
    if (SysBase->LibNode.lib_Version < 36)  /* Fixed (#1139) */
        return 0;
    if (ahimsgport)                     /* made and not deleted by finit:
                                           CloseAHI keeps them (#927) */
        return (ULONG)ahiiorequest;
    ahimsgport = CreateMsgPort();
    if (!ahimsgport)
        return 0;
    ahiiorequest = (struct AHIRequest *)CreateIORequest(ahimsgport,
                                                        sizeof(struct AHIRequest));
    if (!ahiiorequest)
        return 0;
    return (ULONG)ahiiorequest;
}

/**
 * @brief Take back the sound PlaySoundAHI sent: abort it if it still
 * plays, then WaitIO (which also removes its reply from the port).
 *
 * Fixed (#933): the original sent its one request again without waiting
 * for the one before or removing its reply, and finit left everything
 * open while it was in use.  Only a request this library sent with
 * SendIO is waited for, so it never blocks on one the device never had.
 */
static void ahitakeback(void)
{
    if (!ahisent)
        return;
    if (!CheckIO((struct IORequest *)ahiiorequest))
        AbortIO((struct IORequest *)ahiiorequest);
    WaitIO((struct IORequest *)ahiiorequest);
    ahisent = FALSE;
}

/**
 * @brief CloseAHI: stop a running sound, close ahi.device; the request and the port stay.
 *
 * Fixed (#927): the original's CloseAHI was finit, which deleted the
 * request and the port, so that a later OpenAHI used request 0.  Nothing
 * without a request.  A sound that plays is aborted (fixed: #933, the
 * original left everything open while the request was in use);
 * CloseDevice if ahidevice is set (fixed: #924, never set in the
 * original); then ahibase = 0.
 */
static void BLITZ_ROUTINE c_closeahi(void)
{
    if (!ahiiorequest)
        return;
    ahitakeback();
    if (ahidevice) {
        CloseDevice((struct IORequest *)ahiiorequest);
        ahidevice = 0;
    }
    ahibase = NULL;
}

/**
 * @brief finit: close the device (c_closeahi()), delete the request and the port.
 *
 * Nothing without the port; without a request (init could not make it)
 * no CheckIO (fixed: #927, the original's CheckIO of address 0).  A sound
 * that plays is aborted first (fixed: #933).  Then DeleteIORequest,
 * DeleteMsgPort.
 */
static void BLITZ_ROUTINE c_libfinit(void)
{
    if (!ahimsgport)
        return;
    c_closeahi();
    if (ahiiorequest) {
        DeleteIORequest((struct IORequest *)ahiiorequest);
        ahiiorequest = NULL;
    }
    DeleteMsgPort(ahimsgport);
    ahimsgport = NULL;
    ahibase = NULL;
}

/**
 * @brief OpenAHI unit,deviceID: open ahi.device with the library's request.
 *
 * Sets ahir_Version to 4 (MIN_AHI_VERSION) and calls OpenDevice
 * ("ahi.device", unit, request, 0); on success io_Device becomes the
 * library base for the requester calls.  The device ID is stored but
 * not used.  A second OpenAHI while it is open returns 0 (fixed: #924,
 * the original never set ahidevice and opened the open request again).
 * Fixed (#927):
 * without a request (init could not make it) the result is 0; the
 * original wrote ahir_Version to address 48 and passed request 0 to
 * OpenDevice (also after CloseAHI, which deleted the request).
 * @param unit  d0: the AHI unit
 * @param id    d1: the device (audio mode) ID, unused
 * @return d0: BB_TRUE if ahi.device opened, else BB_FALSE
 */
BB_BOOL BLITZ_ROUTINE c_openahi2(ULONG unit BL_REG(d0), ULONG id BL_REG(d1))
{
    struct bl_regs r;

    if (ahidevice)                      /* already open */
        return BB_FALSE;
    deviceunit = unit;
    deviceid = id;                      /* not used */
    if (!ahiiorequest)                  /* Fixed (#927) */
        return BB_FALSE;
    ahiiorequest->ahir_Version = MIN_AHI_VERSION;
    r = (struct bl_regs){ .d0 = unit, .d1 = 0, .a0 = (ULONG)ahiname,
                          .a1 = (ULONG)ahiiorequest };
    if (BLITZ_BASEJSR(SysBase, LVO_OpenDevice, r))
        return BB_FALSE;
    ahibase = ahiiorequest->ahir_Std.io_Device;
    ahidevice = (ULONG)-1;              /* Fixed (#924) */
    return BB_TRUE;
}

/**
 * @brief OpenAHI unit: c_openahi2() with the device ID AHI_DEFAULT_ID.
 * @param unit  d0: the AHI unit
 * @return d0: BB_TRUE if ahi.device opened, else BB_FALSE
 */
BB_BOOL BLITZ_ROUTINE c_openahi(ULONG unit BL_REG(d0))
{
    return c_openahi2(unit, AHI_DEFAULT_ID);
}

/**
 * @brief PlaySoundAHI sound,volume,balance: play a sound object through ahi.device.
 *
 * Without ahi.device (ahidevice, fixed: #924 - the original never set
 * it and never played) returns 0.  Takes back the sound sent before
 * (abort if it still plays, WaitIO; fixed: #933), then CMD_WRITE of the
 * sample as 8 bit mono (AHIST_M8S), the length in bytes = the
 * object's length in words (+6) x 2 (fixed: #932), the frequency 3579545 / the object's period
 * (fixed: #926), ahir_Link 0 and priority 0 (set for every write: the
 * device trashes them, fixed: #924), through SendIO.
 * @param vol  d1: the volume (Fixed 16.16, 0x10000 = full)
 * @param bal  d2: the stereo position (Fixed, 0x8000 = centre)
 * @param obj  a0: audiolib's sound object (!libs #audiolib,#ia0|#pd0)
 * @return d0: 0, or SendIO's d0
 */
ULONG BLITZ_ROUTINE c_playsound2(ULONG vol BL_REG(d1), ULONG bal BL_REG(d2),
                                 struct bb_sound *obj BL_REG(a0))
{
    struct AHIRequest *req = ahiiorequest;
    struct IOStdReq *io = &req->ahir_Std;

    ULONG rc;

    if (!ahidevice)
        return 0;
    ahitakeback();                      /* Fixed (#933) */
    io->io_Command = CMD_WRITE;
    io->io_Flags = 0;
    io->io_Message.mn_Node.ln_Pri = 0;  /* Fixed (#924): trashed by the */
    req->ahir_Link = NULL;              /* device, set for every write */
    io->io_Data = obj->data;
    io->io_Length = (ULONG)obj->len << 1;   /* Fixed (#932): words x 2 */
    io->io_Offset = 0;
    req->ahir_Type = AHIST_M8S;
    req->ahir_Volume = vol;
    req->ahir_Position = bal;
    req->ahir_Frequency = periodfreq(obj->period);  /* Fixed (#926) */
    rc = reqcall(LVO_SendIO, req);
    ahisent = TRUE;                     /* taken back by the next write,
                                           CloseAHI or finit */
    return rc;
}

/**
 * @brief PlaySoundAHI sound,volume: c_playsound2() with the balance centred (0x8000).
 * @param vol  d1: the volume (Fixed 16.16)
 * @param obj  a0: audiolib's sound object
 * @return d0: as c_playsound2()
 */
ULONG BLITZ_ROUTINE c_playsound1(ULONG vol BL_REG(d1), struct bb_sound *obj BL_REG(a0))
{
    return c_playsound2(vol, 0x8000, obj);
}

/**
 * @brief PlaySoundAHI sound: c_playsound2() with full volume (0x10000), centred.
 * @param obj  a0: audiolib's sound object
 * @return d0: as c_playsound2()
 */
ULONG BLITZ_ROUTINE c_playsound(struct bb_sound *obj BL_REG(a0))
{
    return c_playsound2(0x10000, 0x8000, obj);
}

/**
 * @brief PlayRawAHI address,length,frequency,volume,balance: play raw 8 bit data through ahi.device.
 *
 * Without ahi.device (ahidevice, fixed: #924 - the original never set
 * it and never played) returns 0.  Takes back a sound of PlaySoundAHI
 * (fixed: #933), then CMD_WRITE as AHIST_M8S with ahir_Link 0 and
 * priority 0 (fixed: #924) through DoIO: it returns when the data has
 * been played (the data may be a temporary buffer).
 * @param data  d0: the sample data
 * @param len   d1: its length in bytes
 * @param freq  d2: the frequency
 * @param vol   d3: the volume (Fixed 16.16)
 * @param bal   d4: the stereo position (Fixed)
 * @return d0: 0, or DoIO's d0
 */
ULONG BLITZ_ROUTINE c_playraw2(ULONG data BL_REG(d0), ULONG len BL_REG(d1),
                               ULONG freq BL_REG(d2), ULONG vol BL_REG(d3),
                               ULONG bal BL_REG(d4))
{
    struct AHIRequest *req = ahiiorequest;
    struct IOStdReq *io = &req->ahir_Std;

    if (!ahidevice)
        return 0;
    ahitakeback();                      /* Fixed (#933) */
    io->io_Command = CMD_WRITE;
    io->io_Flags = 0;
    io->io_Message.mn_Node.ln_Pri = 0;  /* Fixed (#924) */
    req->ahir_Link = NULL;
    io->io_Data = (APTR)data;
    io->io_Length = len;
    io->io_Offset = 0;
    req->ahir_Type = AHIST_M8S;
    req->ahir_Volume = vol;
    req->ahir_Position = bal;
    req->ahir_Frequency = freq;
    return reqcall(LVO_DoIO, req);
}

/**
 * @brief PlayRawAHI address,length,frequency,volume: c_playraw2() centred (0x8000).
 * @param data  d0: the sample data
 * @param len   d1: its length in bytes
 * @param freq  d2: the frequency
 * @param vol   d3: the volume (Fixed 16.16)
 * @return d0: as c_playraw2()
 */
ULONG BLITZ_ROUTINE c_playraw1(ULONG data BL_REG(d0), ULONG len BL_REG(d1),
                               ULONG freq BL_REG(d2), ULONG vol BL_REG(d3))
{
    return c_playraw2(data, len, freq, vol, 0x8000);
}

/**
 * @brief PlayRawAHI address,length,frequency: c_playraw2() at full volume, centred.
 * @param data  d0: the sample data
 * @param len   d1: its length in bytes
 * @param freq  d2: the frequency
 * @return d0: as c_playraw2()
 */
ULONG BLITZ_ROUTINE c_playraw(ULONG data BL_REG(d0), ULONG len BL_REG(d1),
                              ULONG freq BL_REG(d2))
{
    return c_playraw2(data, len, freq, 0x10000, 0x8000);
}

/**
 * @brief AHIOutputRequest x,y,width,height,modeid,frequency: AHI's audio mode requester.
 *
 * Writes the arguments into the data of the tag list's initial left
 * edge, top edge, width, height, audio id and mix frequency (offsets 4,
 * 12, 20, 28, 36, 44; fixed: #931, the original wrote the last four to
 * 16, 20, 24, 28, over the tag ids of width and height), then
 * AHI_AllocAudioRequestA and AHI_AudioRequestA (no tags).
 * BUG(orig): !libs's screen (a0) is not used,
 * AHIR_Screen stays 0 (#929).  Every call allocates a requester that is
 * never freed (#930).
 * @param x     d0: the initial left edge
 * @param y     d1: the initial top edge
 * @param w     d2: the initial width
 * @param h     d3: the initial height
 * @param id    d4: the initial audio mode id
 * @param freq  d5: the initial mixing frequency
 * @return d0: AHI_AudioRequestA's result (0 when cancelled), 0 without
 *         ahi.device or when the requester could not be allocated
 */
ULONG BLITZ_ROUTINE c_outreq6(ULONG x BL_REG(d0), ULONG y BL_REG(d1), ULONG w BL_REG(d2),
                              ULONG h BL_REG(d3), ULONG id BL_REG(d4), ULONG freq BL_REG(d5))
{
    struct bl_regs r;

    if (!ahibase)
        return 0;
    ahireqtags.left.ti_Data = x;        /* 4(a0) */
    ahireqtags.top.ti_Data = y;         /* 12(a0) */
    ahireqtags.width.ti_Data = w;       /* Fixed (#931): 20(a0) */
    ahireqtags.height.ti_Data = h;      /* 28(a0) */
    ahireqtags.audioid.ti_Data = id;    /* 36(a0) */
    ahireqtags.mixfreq.ti_Data = freq;  /* 44(a0) */
    r = (struct bl_regs){ .a0 = (ULONG)&ahireqtags };
    ahirequest = (APTR)BLITZ_BASEJSR(ahibase, LVO_AHI_AllocAudioRequestA, r);
    if (!ahirequest)
        return 0;
    r = (struct bl_regs){ .a0 = (ULONG)ahirequest, .a1 = 0 };
    return BLITZ_BASEJSR(ahibase, LVO_AHI_AudioRequestA, r);
}

/**
 * @brief AHIOutputRequest: c_outreq6() with x, y, width, height -1 and mode id, frequency 0.
 * @return d0: as c_outreq6()
 */
ULONG BLITZ_ROUTINE c_outreq1(void)
{
    return c_outreq6((ULONG)-1, (ULONG)-1, (ULONG)-1, (ULONG)-1, 0, 0);
}

/**
 * @brief The entry "AudioID": c_outreq6() with x, y, width, height -1.
 *
 * BUG(orig) (#928): !subs gives it to the form with 4 arguments
 * (x,y,width,height): those are replaced by -1, the mode id is whatever
 * d4 holds.
 * @param id  d4: the mode id (the caller's d4)
 * @return d0: as c_outreq6()
 */
ULONG BLITZ_ROUTINE c_outreq2(ULONG id BL_REG(d4))
{
    return c_outreq6((ULONG)-1, (ULONG)-1, (ULONG)-1, (ULONG)-1, id, 0);
}

/**
 * @brief The entry "AudioID, Frequency": c_outreq6() with x, y, width, height -1.
 *
 * BUG(orig) (#928): !subs gives it to the form with 1 argument: the
 * argument (d0) is replaced, mode id and frequency are whatever d4/d5
 * hold.
 * @param id    d4: the mode id (the caller's d4)
 * @param freq  d5: the frequency (the caller's d5)
 * @return d0: as c_outreq6()
 */
ULONG BLITZ_ROUTINE c_outreq3(ULONG id BL_REG(d4), ULONG freq BL_REG(d5))
{
    return c_outreq6((ULONG)-1, (ULONG)-1, (ULONG)-1, (ULONG)-1, id, freq);
}

/**
 * @brief The entry "X,Y,WIDTH,HEIGHT": c_outreq6() with mode id and frequency 0.
 *
 * BUG(orig) (#928): !subs gives it to the form with 2 arguments: width
 * and height are whatever d2/d3 hold.
 * @param x  d0: the left edge
 * @param y  d1: the top edge
 * @param w  d2: the width (the caller's d2)
 * @param h  d3: the height (the caller's d3)
 * @return d0: as c_outreq6()
 */
ULONG BLITZ_ROUTINE c_outreq4(ULONG x BL_REG(d0), ULONG y BL_REG(d1), ULONG w BL_REG(d2),
                              ULONG h BL_REG(d3))
{
    return c_outreq6(x, y, w, h, 0, 0);
}

/**
 * @brief AHIOutputRequest x,y,width,height,modeid: c_outreq6() with frequency 0.
 * @param x   d0: the left edge
 * @param y   d1: the top edge
 * @param w   d2: the width
 * @param h   d3: the height
 * @param id  d4: the mode id
 * @return d0: as c_outreq6()
 */
ULONG BLITZ_ROUTINE c_outreq5(ULONG x BL_REG(d0), ULONG y BL_REG(d1), ULONG w BL_REG(d2),
                              ULONG h BL_REG(d3), ULONG id BL_REG(d4))
{
    return c_outreq6(x, y, w, h, id, 0);
}

/**
 * @brief AHIRequestedID: the audio mode id chosen in AHIOutputRequest.
 *
 * BUG(orig) (#934): returns the data of the initial tag
 * AHIR_InitialAudioID (-1, never written), not the requester's
 * ahiam_AudioID.
 * @return d0: the tag's data, 0 without ahi.device
 */
ULONG BLITZ_ROUTINE c_requestedid(void)
{
    if (!ahibase)
        return 0;
    return ahireqtags.audioid.ti_Data;  /* ahiaudioid+4: the initial value, not
                                           the requester's ahiam_AudioID */
}

/**
 * @brief AHIRequestedFreq: the mixing frequency chosen in AHIOutputRequest.
 *
 * BUG(orig) (#934): returns the data of the initial tag
 * AHIR_InitialMixFreq (0, never written), not the requester's
 * ahiam_MixFreq.
 * @return d0: the tag's data, 0 without ahi.device
 */
ULONG BLITZ_ROUTINE c_requestedfreq(void)
{
    if (!ahibase)
        return 0;
    return ahireqtags.mixfreq.ti_Data;  /* ahifrequency+4: the initial value,
                                           not the requester's ahiam_MixFreq */
}

/**
 * @brief CludgeSound sound#,address: make a sound object of an 8SVX file in memory.
 *
 * risoundlib's routine: checks "8SVX" at +8 and "VHDR" at +12, takes the
 * rate from the VHDR, the period 3579545 / rate, finds the BODY among the
 * first seven chunks, frees the object's old sample (memlib globfree; not
 * DecodeSound's in-place "cL" data, fixed: #945; after the checks, the
 * pointer cleared, fixed: #831) and copies the BODY to chip memory
 * (memlib globalloc).  Fixed (#926): nothing is written behind the 16
 * byte object (the original's rate at +16).  Fixed (#925): mark
 * (+14) is cleared again, as in risoundlib; Fixed (#832): the rate is
 * samplesPerSec (VHDR data + 12); Fixed (#830): rate 0 gives 0; Fixed
 * (#828): the chunk walk skips the pad bytes and stays inside the FORM;
 * Fixed (#829): the whole BODY is copied, an empty one or one of 131072
 * bytes or more gives 0.
 * @param sound  d0: sound# (its upper word goes into globfree's d0)
 * @param sram   d1: the 8SVX file in memory
 * @param saddr  a0: audiolib's sound object
 * @return d0: BB_TRUE if the sound was made, BB_FALSE if the file is no
 *         8SVX with a VHDR, its rate is 0, it has no BODY among the first
 *         seven chunks of the FORM, the BODY is empty or 131072 bytes or
 *         longer (the sound unchanged), or no chip memory
 *         (the sound then has no sample)
 */
static BB_BOOL BLITZ_ROUTINE loadsounds_c(ULONG sound BL_REG(d0), UBYTE *sram BL_REG(d1),
                                          struct bb_sound *saddr BL_REG(a0))
{
    UBYTE *iff, *src, *dst, *mem;       /* a0: the current IFF chunk */
    ULONG rate, clock, slen, formlen, off, len;
    UWORD sper;
    int chunk;

    if (LONGAT(sram + 8) != ID('8', 'S', 'V', 'X') || LONGAT(sram + 12) != ID('V', 'H', 'D', 'R'))
        return BB_FALSE;
    rate = WORDAT(sram + 12 + 8 + 12);  /* Fixed (#832): samplesPerSec, not
                                           the word at VHDR + its length */
    if (!rate)                          /* Fixed (#830): the loop never ended */
        return BB_FALSE;
    clock = 3579545;                    /* CMP.l d1,d0:BLT: signed */
    sper = 0;
    while ((LONG)clock >= (LONG)rate) {
        sper++;
        clock -= rate;
    }
    /* Fixed (#828): the walk skips the pad byte behind a chunk of odd
       length and stays inside the FORM (a chunk that does not fit ends
       it with 0) */
    formlen = LONGAT(sram + 4);
    off = 12;
    for (chunk = 0; ; ) {
        if (off > formlen)              /* the chunk header outside the FORM */
            return BB_FALSE;
        if (LONGAT(sram + off) == ID('B', 'O', 'D', 'Y'))
            break;
        len = LONGAT(sram + off + 4);
        if (len > formlen - off)        /* the chunk outside the FORM */
            return BB_FALSE;
        off += 8 + len + (len & 1);
        if (++chunk > 6)
            return BB_FALSE;
    }
    iff = sram + off;
    src = iff + 8;
    slen = LONGAT(iff + 4);
    if (!slen || slen > MAXBODY)        /* Fixed (#829): none, or more than
                                           the word at +6 can describe */
        return BB_FALSE;
    /* Fixed (#831): the old sample is freed after the checks, and the
       object no longer points to it (a failed check leaves the sound as
       it was) */
    if (saddr->data && saddr->mark != BB_CLUDGE) {
                                        /* Fixed (#945): not DecodeSound's
                                           in-place ("cL") data */
        struct bl_regs r = { .a1 = (ULONG)saddr->data };
        r.d0 = BL_SETW(sound, saddr->len);
        BLITZ_ALIBJSR(TOK_globfree, r);
    }
    saddr->data = NULL;
    {
        struct bl_regs r = { .d0 = slen, .d1 = MEMF_CHIP };
        mem = dst = (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
    }
    if (!mem)
        return BB_FALSE;
    for (len = slen; len; len--)        /* Fixed (#829): all of it */
        *dst++ = *src++;
    saddr->data = mem;
    saddr->period = sper;
    saddr->len = (UWORD)(slen >> 1);
    saddr->rep = NULL;                  /* no loop */
    saddr->replen = 0;
    saddr->mark = 0;                    /* Fixed (#925): as risoundlib */
                                        /* Fixed (#926): no rate at +16 */
    return BB_TRUE;
}

/* Registers: the originals change d0/d1/a0/a1 (exec, AHI, memlib), a6
   and AHIOutputRequest d2-d5 as well; the thunks keep d1/a0/a1. */
BLITZ_THUNK(risoundlib2_loadsounds, loadsounds_c, "d1/a0-a1");
BLITZ_THUNK(risoundlib2_libinit, c_libinit, "d1/a0-a1");
BLITZ_THUNK(risoundlib2_libfinit, c_libfinit, "d0-d1/a0-a1");
BLITZ_THUNK(_CloseAHI, c_closeahi, "d0-d1/a0-a1");
BLITZ_THUNK(_OpenAHI, c_openahi, "d1/a0-a1");
BLITZ_THUNK(_OpenAHI2, c_openahi2, "d1/a0-a1");
BLITZ_THUNK(_PlaySoundAHI, c_playsound, "d1/a0-a1");
BLITZ_THUNK(_PlaySoundAHI1, c_playsound1, "d1/a0-a1");
BLITZ_THUNK(_PlaySoundAHI2, c_playsound2, "d1/a0-a1");
BLITZ_THUNK(_PlayRawAHI, c_playraw, "d1/a0-a1");
BLITZ_THUNK(_PlayRawAHI1, c_playraw1, "d1/a0-a1");
BLITZ_THUNK(_PlayRawAHI2, c_playraw2, "d1/a0-a1");
BLITZ_THUNK(_AHIOutputRequest1, c_outreq1, "d1/a0-a1");
BLITZ_THUNK(_AHIOutputRequest2, c_outreq2, "d1/a0-a1");
BLITZ_THUNK(_AHIOutputRequest3, c_outreq3, "d1/a0-a1");
BLITZ_THUNK(_AHIOutputRequest4, c_outreq4, "d1/a0-a1");
BLITZ_THUNK(_AHIOutputRequest5, c_outreq5, "d1/a0-a1");
BLITZ_THUNK(_AHIOutputRequest6, c_outreq6, "d1/a0-a1");
BLITZ_THUNK(_AHIRequestedID, c_requestedid, "d1/a0-a1");
BLITZ_THUNK(_AHIRequestedFreq, c_requestedfreq, "d1/a0-a1");
