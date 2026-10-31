/*
 * rawkeylib - command library 130: raw key code <-> ASCII for other
 * libraries (no tokens of its own; windowslib, gameiolib ... call it):
 *   +1 convertkey   d0 = raw code, d1 = qualifier -> d0 = character
 *   +2 unconvert    d0 = character -> d0 = raw code (-1), d1 = qualifier
 *   +3 qconvertkey  d0 = raw code, d1 = qualifier -> d0 = character
 *                   (table look-up, no console.device)
 *
 * C translation of bblibsrc/rawkeylib; the source INCLUDEs
 * libincs:rawcode, which is not on the CD: everything except unconvert
 * is translated from the binary of the CD's acidlibs (ref/rawkeylib.obj).
 *
 * init opens console.device (unit -1, own IOStdReq and MsgPort in the
 * library, signal from AllocSignal) and fills two tables of 105 entries
 * (raw codes 0-104) with convertkey(code, 0) and convertkey(code, 1)
 * (shift).  finit frees the signal and closes the device.
 *
 * convertkey: F1-F10 ($50-$59) -> 129-138, cursor keys ($4C-$4F) ->
 * 28-31, Help ($5F) -> 139, everything else through RawKeyConvert (one
 * character, keymap NULL, qualifier as given; 0 if nothing comes, the
 * buffer's first byte also on an error).  Results only in the low word
 * of d0 (MOVE.w / ADDI.w): the upper word is the caller's (fixed codes)
 * or the one of RawKeyConvert's result.  The RawKeyConvert path also
 * leaves d1 = the character (kept: thunk).
 * qconvertkey: table normal (qualifier & 3 = 0) or shifted; with CTRL
 * (bit 3) A-Z/a-z -> 1-26 and the other characters 65-122 except 91-96
 * (`AND #$FFDF:SUB #64`: [ \ ] ^ _ ` stay), else CAPS LOCK (bit 2) makes
 * a-z upper case.
 *
 * BUG(orig) (#245), kept: qconvertkey indexes the table with the raw code & 255
 * - codes 105-255 (key-up codes $80+ included) read on behind the table:
 * the shifted table, then the library's own code (qconvertkey and
 * convertkey).  Those 151 code bytes of the original are appended to the
 * tables as data, so that the C library returns the same values.
 * init's result (long, stored by the compiler, used by nobody) is d0 of
 * the last convertkey call (code 104, shifted).
 * unconvert looks through both tables in one run (normal, then shifted;
 * d1 = 0/1), so a character that exists in both gives the unshifted key.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <exec/io.h>
#include <exec/ports.h>
#include <devices/inputevent.h>
#define __NOLIBBASE__
#include <proto/console.h>
#undef __NOLIBBASE__

/* entry points (thunks / C routines, defined below) */
void convertkey(void), unconvert(void), qconvertkey(void);
ULONG BLITZ_ROUTINE codeinit(void);
void BLITZ_ROUTINE codefinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader{#rawkeylib,init,1,finit,0} */
BLITZ_LIBHEADER(LIB_rawkeylib, init, BL_LONGRET, finit, 0);

BLITZ_NULLSUB(convertkey, 0, 0);
BLITZ_NULLSUB(unconvert, 0, 0);
BLITZ_NULLSUB(qconvertkey, 0, 0);
BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);

BLITZ_LIBFIN();

/* ------------------------------------------------------------ variables */

/* the two tables, followed by the original's code bytes $1CC-$262
   (qconvertkey reads up to table + 360, see BUG(orig) (#245) above) */
static struct BL_PACKED {
    UBYTE normmap[105];
    UBYTE shiftmap[105];
    UBYTE code[151];
} maps = {
    .code = {
        0x48, 0xe7, 0x20, 0x80, 0x41, 0xfa, 0xff, 0x28, 0x34, 0x01, 0xc4, 0x7c,
        0x00, 0x03, 0x67, 0x00, 0x00, 0x06, 0x41, 0xfa, 0xff, 0x83, 0xc0, 0x7c,
        0x00, 0xff, 0x10, 0x30, 0x00, 0x00, 0x67, 0x00, 0x00, 0x52, 0x08, 0x01,
        0x00, 0x03, 0x67, 0x00, 0x00, 0x2e, 0x0c, 0x40, 0x00, 0x41, 0x65, 0x00,
        0x00, 0x42, 0x0c, 0x40, 0x00, 0x7a, 0x62, 0x00, 0x00, 0x3a, 0x0c, 0x40,
        0x00, 0x5a, 0x63, 0x00, 0x00, 0x0a, 0x0c, 0x40, 0x00, 0x61, 0x65, 0x00,
        0x00, 0x2a, 0xc0, 0x7c, 0xff, 0xdf, 0x04, 0x40, 0x00, 0x40, 0x60, 0x00,
        0x00, 0x1e, 0x0c, 0x00, 0x00, 0x61, 0x65, 0x00, 0x00, 0x16, 0x0c, 0x00,
        0x00, 0x7a, 0x62, 0x00, 0x00, 0x0e, 0x08, 0x01, 0x00, 0x02, 0x67, 0x00,
        0x00, 0x06, 0x04, 0x40, 0x00, 0x20, 0x4c, 0xdf, 0x01, 0x04, 0x4e, 0x75,
        0x0c, 0x40, 0x00, 0x59, 0x62, 0x00, 0x00, 0x1e, 0x0c, 0x40, 0x00, 0x50,
        0x65, 0x00, 0x00, 0x08, 0x06, 0x40, 0x00, 0x31, 0x4e, 0x75, 0x0c, 0x40,
        0x00, 0x4c, 0x65, 0x00, 0x00, 0x16, 0x04,
    },
};

/* IOStdReq and MsgPort in one block (init clears both, 82 bytes) */
static struct BL_PACKED {
    struct IOStdReq io;
    struct MsgPort port;
} con;

static struct Library *ConsoleDevice;
static UWORD buf;                               /* RawKeyConvert buffer */
static struct InputEvent event = { .ie_Class = IECLASS_RAWKEY };

/* second results in d1 (set by the thunks' tail) */
ULONG rawkey_d1;

/* ---------------------------------------------------------- convertkey */

/**
 * @brief convertkey (+1, for other libraries) - raw key code to a character
 * through console.device.
 *
 * F1-F10 ($50-$59) -> 129-138, cursor keys ($4C-$4F) -> 28-31, Help
 * ($5F) -> 139; everything else through RawKeyConvert (one character,
 * keymap NULL, the qualifier as given; 0 if nothing comes, the buffer's
 * first byte also on an error).  The result is only in the low word of d0
 * (MOVE.w / ADDI.w): the upper word is the caller's (fixed codes) or the
 * one of RawKeyConvert's result.
 *
 * @param key  d0: the raw key code (low word)
 * @param qual d1: the qualifier
 * @return d0.w: the character; d1 = the character on the RawKeyConvert
 *         path, else the qualifier (set by the thunk)
 */
static ULONG BLITZ_ROUTINE convertkey_c(ULONG key BL_REG(d0), ULONG qual BL_REG(d1))
{
    UWORD c = key;
    LONG r;
    ULONG ch;                                   /* d1: the converted character */

    rawkey_d1 = qual;
    if (c <= 89) {                              /* CMP #89,d0:BHI */
        if (c >= 80)                            /* F1-F10 */
            return (key & 0xffff0000) | (UWORD)(c + 49);
        if (c >= 76)                            /* cursor keys */
            return (key & 0xffff0000) | (UWORD)(c - 48);
    } else if (c == 95)                         /* Help */
        return (key & 0xffff0000) | 139;
    event.ie_Code = c;
    event.ie_Qualifier = qual;
    buf = 0;
    r = RawKeyConvert(&event, (STRPTR)&buf, 1, NULL);
    ch = 0;
    if ((UWORD)r != 0xffff)                     /* CMP #-1,d0 (word) */
        ch = *(UBYTE *)&buf;
    rawkey_d1 = ch;
    return ((ULONG)r & 0xffff0000) | (UWORD)ch; /* MOVE d1,d0 */
}
BLITZ_THUNK_POST(convertkey, convertkey_c, "a0-a1", "move.l  _rawkey_d1,d1");

/* ----------------------------------------------------------- unconvert */

/**
 * @brief unconvert (+2, for other libraries) - character to raw key code.
 *
 * Looks through both tables in one run (normal, then shifted), so a
 * character that exists in both gives the unshifted key.
 *
 * @param ch d0.b: the character
 * @return d0: the raw key code, -1 if not found; d1 = 0 (normal table) or
 *         1 (shifted table, also when not found)
 */
static ULONG BLITZ_ROUTINE unconvert_c(ULONG ch BL_REG(d0))
{
    const UBYTE *p = maps.normmap;
    UWORD i;

    for (i = 0; i < 105; i++)                   /* CMP.b (a0)+,d0:DBEQ */
        if (*p++ == (UBYTE)ch) {
            rawkey_d1 = 0;
            return i;                           /* MOVEQ #104,d0:SUB d2,d0 */
        }
    for (i = 0; i < 105; i++)                   /* goes on into shiftmap */
        if (*p++ == (UBYTE)ch) {
            rawkey_d1 = 1;
            return i;
        }
    rawkey_d1 = 1;
    return (ULONG)-1;
}
BLITZ_THUNK_POST(unconvert, unconvert_c, "a0-a1", "move.l  _rawkey_d1,d1");

/* --------------------------------------------------------- qconvertkey */

/**
 * @brief qconvertkey (+3, for other libraries) - raw key code to a
 * character by table look-up (no console.device).
 *
 * The normal table (qualifier & 3 = 0) or the shifted one; with CTRL
 * (bit 3) A-Z/a-z -> 1-26 and the other characters 65-122 except 91-96
 * (`AND #$FFDF:SUB #64`: [ \ ] ^ _ ` stay), else CAPS LOCK (bit 2) makes
 * a-z upper case.  Only the low word of d0 changes.
 *
 * BUG(orig) (#245): the table is indexed with the raw code & 255 - codes
 * 105-255 (key-up codes $80+ included) read on behind the table: the
 * shifted table, then the library's own code (the bytes of the original
 * are appended to the tables).
 *
 * @param key  d0: the raw key code (low word)
 * @param qual d1: the qualifier
 * @return d0.w: the character (0 if none)
 */
static ULONG BLITZ_ROUTINE qconvertkey_c(ULONG key BL_REG(d0), ULONG qual BL_REG(d1))
{
    const UBYTE *map = (qual & 3) ? maps.shiftmap : maps.normmap;
    UWORD w;

    BL_SETW(key, key & 255);                    /* AND #255,d0 */
    BL_SETB(key, map[(UWORD)key]);              /* MOVE.b 0(a0,d0),d0 */
    w = key;
    if (!(UBYTE)w)
        return key;
    if (qual & 8) {                             /* CTRL */
        if (w < 65 || w > 122)
            return key;
        if (w > 90 && w < 97)
            return key;
        BL_SETW(key, (w & 0xffdf) - 64);
        return key;
    }
    if ((UBYTE)w < 97 || (UBYTE)w > 122)
        return key;
    if (qual & 4)                               /* CAPS LOCK */
        BL_SETW(key, w - 32);
    return key;
}
BLITZ_THUNK(qconvertkey, qconvertkey_c, "d1/a0-a1");

/* ------------------------------------------------------------ init/finit */

/**
 * @brief Library init: open console.device and fill the two tables.
 *
 * Clears the IOStdReq and MsgPort, sets them up (signal from AllocSignal),
 * opens console.device unit -1 and fills the tables for the raw codes
 * 0-104 with convertkey(code, 0) and convertkey(code, 1) (shift).  Neither
 * the signal nor OpenDevice is checked.
 *
 * @return d0: d0 of the last convertkey call (code 104, shifted); stored
 *         by the compiler, used by nobody
 */
ULONG BLITZ_ROUTINE codeinit(void)
{
    UBYTE *p = (UBYTE *)&con;
    struct List *l = &con.port.mp_MsgList;
    ULONG openerr;                              /* d0 */
    ULONG key;                                  /* d0: its upper word carries over */
    UWORD i;

    for (i = 0; i < sizeof(con); i++)           /* 41 words cleared */
        p[i] = 0;
    con.io.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    con.io.io_Message.mn_Length = sizeof(struct IOStdReq);
    l->lh_TailPred = (struct Node *)&l->lh_Head;
    l->lh_Head = (struct Node *)&l->lh_Tail;
    con.port.mp_Node.ln_Type = NT_MSGPORT;
    con.port.mp_SigTask = FindTask(NULL);
    con.port.mp_SigBit = AllocSignal(-1);
    con.io.io_Message.mn_ReplyPort = &con.port;
    openerr = OpenDevice("console.device", -1, (struct IORequest *)&con.io, 0);
    ConsoleDevice = (struct Library *)con.io.io_Device;
    key = openerr;
    for (i = 0; i < 105; i++) {
        key = (key & 0xffff0000) | i;           /* MOVE d2,d0 */
        key = convertkey_c(key, 0);
        maps.normmap[i] = key;
        key = (key & 0xffff0000) | i;
        key = convertkey_c(key, 1);
        maps.shiftmap[i] = key;
    }
    return key;
}

/** @brief Library finit: free the signal and close console.device. */
void BLITZ_ROUTINE codefinit(void)
{
    FreeSignal((UBYTE)con.port.mp_SigBit);      /* MOVEQ #0,d0:MOVE.b */
    CloseDevice((struct IORequest *)&con.io);
}
