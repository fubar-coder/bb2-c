/*
 * rawkey.c - rawed.obj (ted.s:7109, incbin of a hunk file without source)
 * in C: see rawkey.h and reference.md 3.1.  Translated from the DSM
 * disassembly bb2src.zip.dir/rawed.obj.dsm (labels L1-L17 there; offsets
 * relative to rawed+32).  The special key compares are core: rawspecial
 * (keymap.c).
 */
#include <string.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include <proto/console.h>
#include "ted.h"

/* ---------------------------------------- the data of the hunk (code+$0C) */

/* L1 (+$0C): device name */
static const char rawed_devname[] = "console.device";
/* L2 (+$1C): device base, = io_Device after OpenDevice (proto/console.h) */
struct Device *ConsoleDevice;
/* L3 (+$20): output buffer of RawKeyConvert (word, cleared per call) */
static UBYTE rawed_buf[2];
/* L4 (+$22): the input event: ie_Class = 1 (RAWKEY), everything else 0;
   convert writes ie_Code (+6) and ie_Qualifier (+8) only */
static struct InputEvent rawed_ie = { NULL, IECLASS_RAWKEY };
/* L5/L6 (+$38/+$A1): rawkeys $00-$68 converted without/with qualifier 1
   (left shift).  Built by init, never read (neither by rawed
   nor by ted.s) - kept for the same side effects (210 RawKeyConvert
   calls, decision O5). */
static UBYTE rawed_tab1[105], rawed_tab2[105];
/* L15 (+$218) IOStdReq (48 bytes), L16 (+$248) MsgPort (34 bytes): one
   block of 82 bytes in the hunk, cleared by init */
static struct {
    struct IOStdReq io;
    struct MsgPort port;
} rawed_io;

/* rawed+32+0 (called at ted.s line 39): L12 */
BOOL rawed_init(void)
{
    struct IOStdReq *io = &rawed_io.io;
    struct MsgPort *port = &rawed_io.port;
    struct List *list;
    UWORD key;

    memset(&rawed_io, 0, sizeof(rawed_io));     /* 41 words */
    io->io_Message.mn_Node.ln_Type = NT_MESSAGE;        /* 5 */
    io->io_Message.mn_Length = sizeof(struct IOStdReq); /* $30 */
    list = &port->mp_MsgList;   /* NewList by hand */
    list->lh_TailPred = (struct Node *)list;
    list->lh_Head = (struct Node *)&list->lh_Tail;
    port->mp_Node.ln_Type = NT_MSGPORT;                 /* 4 */
    port->mp_SigTask = FindTask(NULL);
    /* the signal is never freed (finit does not; ends with the process) */
    port->mp_SigBit = (UBYTE)AllocSignal(-1);
    io->io_Message.mn_ReplyPort = port;
    /* Fixed (#373): the result is checked (the original went on with
       ConsoleDevice = 0 and convert called through a NULL base) */
    if (OpenDevice((CONST_STRPTR)rawed_devname, (ULONG)-1,
                   (struct IORequest *)io, 0) != 0) {
        FreeSignal((BYTE)port->mp_SigBit);
        ConsoleDevice = NULL;
        return FALSE;
    }
    ConsoleDevice = io->io_Device;
    /* L14: both (unused) tables through convert */
    for (key = 0; key != 0x69; key++) {
        rawed_tab1[key] = (UBYTE)rawed_convert(key, 0);
        rawed_tab2[key] = (UBYTE)rawed_convert(key, 1);
    }
    return TRUE;
}

/* rawed+32+4 (called at ted.s line 5928): L7 */
UWORD rawed_convert(UWORD code, UWORD qual)
{
    UWORD sp = rawspecial(code);
    LONG r;

    if (sp)                     /* F1-F10, cursor keys, Help (any qualifier) */
        return sp;
    /* L10 */
    rawed_ie.ie_Code = code;
    rawed_ie.ie_Qualifier = qual;
    rawed_buf[0] = rawed_buf[1] = 0;
    r = RawKeyConvert(&rawed_ie, (STRPTR)rawed_buf, 1, NULL);
    /* L11 (d1) */
    if ((WORD)r != -1)          /* cmpi.w #-1,d0: buffer overflow -> 0 */
        return rawed_buf[0];    /* 0 if no character */
    return 0;
}

/* rawed+32+8 (called from the fail chain, ted.s line 128): CloseDevice
   (tail jmp into exec; the signal is not freed) */
void rawed_finit(void)
{
    CloseDevice((struct IORequest *)&rawed_io.io);
}
