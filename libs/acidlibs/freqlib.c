/*
 * freqlib - command library 152: FileRequest$ - the file requester of
 * Blitz Basic 2 (its own window with a directory list, DRIVES, PARENT,
 * OK, CANCEL, path and file string gadgets).
 *
 * C translation of bblibsrc/freqlib (the wrapper dofreq and the error
 * check) and of the requester itself, which the source includes from
 * libincs:filereq.s (not on the CD: translated from the disassembly of
 * the CD binary).  The requester's data - NewWindow, gadgets, borders,
 * IntuiTexts, the 64 list line gadgets (144 bytes each: Gadget, IntuiText,
 * 80 character text), string buffers and its variables - keep the layout
 * of the original: they are one data block, the variable fq (struct
 * fq_data, NDK structures and fields at the original offsets, checked by
 * static_assert; contents and relocations as in the binary).
 *
 * FileRequest$(title$,path$,file$): path$ and file$ are string variables
 * (#varptr, buffers of 160/64 bytes - checked by the debugger).  OK
 * (button, double click on a file, Return in the file gadget) copies the
 * path and the file name into them and returns path + "/" (unless it
 * ends in ":" or "/") + file; CANCEL and the close gadget return "".
 * The window remembers its position and size for the next call; its
 * height is the screen height - 40 the first time.
 *
 * The requester core (code, data block, parameter block fq_args) is
 * shared/filereq/filereq_core.c, shared with TED (ted-c/src/filereq.c) and
 * compiled into this library with its options (CMakeLists.txt); this file
 * is the BlitzLib wrapper around it.  The list below covers both parts.
 *
 * Kept from the original (BUG(orig)):
 *   - path$/file$ get the new contents but not their new lengths. (#153)
 *   - dos, intuition and graphics are opened (OldOpenLibrary) at every
 *     call and never closed. (#154)
 *   - The volume list (DRIVES) walks the DOS list without locking it
 *     (the labels are right: dl_Type 1 = DLT_DIRECTORY -> $F9 "<ASN>",
 *     2 = DLT_VOLUME -> $F8 "<VOL>"; late/non-binding assigns are not
 *     listed). (#156)
 *   - "CD" (the gadget at $FB6) makes the path the current directory of
 *     the process and unlocks the old current directory (#157); it also
 *     clears the high word of the path gadget's UndoBuffer pointer ($111A
 *     instead of DispPos, details in filereq_core.c) (#345).
 *   - Building the path from the current directory leaks the ParentDir
 *     locks. (#158)
 *   - List entries are 40 byte nodes (AllocMem, not checked); names longer
 *     than 34 characters overrun them. (#159)
 *   - There are 64 line gadgets (144 bytes each, $15AA-$39A9, then 48
 *     bytes $39AA-$39D9 up to the end of the data block), but the window shows
 *     (height - 85) / 8 lines with no limit: from a window height of 605
 *     the layout writes the line gadgets beyond the data block into
 *     whatever follows the library in the program.  The first window is screen height - 40 high, so this
 *     happens on every screen of 645 lines or more (MaxHeight is $FFFF,
 *     so also after enlarging the window).  Measured in the host harness
 *     (screen height 1050: window 1010, 115 lines, 51 too many, ~7.2 KB
 *     written past $39DA; screen 256: nothing): FileRequest$ itself works,
 *     the program crashes afterwards - in the emulator (RTG Workbench
 *     1680x1050) right after the requester returned, with cancel and with
 *     OK (t93/t94: reboot, earlier "ILLEGAL Instruction").  Stack, a2-a6,
 *     pr_WindowPtr, locks and memory are all in order on return.  Kept:
 *     the C version writes past fq.line[63] and fq in the same way. (#160)
 *   - A result of a3 + length + 1: a3 is left BEHIND the terminating 0. (#161)
 * DIFF: if the chip memory or a library or the window cannot be opened,
 *   the original returns the first long of its FileInfoBlock buffer on
 *   the stack (uninitialized); here 0.
 * Registers: the requester keeps d1-a6 (MOVEM) - the thunk keeps d1/a1
 * (the original changes a0 and a3).
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (defined below) */
void dofreq(void), freqlib_freqchk(void);

/* !libheader{#freqlib,0,0,0,errors} */
BLITZ_LIBHEADER(LIB_freqlib, 0, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, BL_VARPTR | BL_STRING, BL_VARPTR | BL_STRING);
BLITZ_LIBS(LIB_screenslib, 0x1102);
BLITZ_SUBS(BLITZ_AMIGAONLY(dofreq), freqlib_freqchk, 0);
BLITZ_NAME("FileRequest$", "Title$,Pathname$,Filename$");

BLITZ_LIBFIN();

/* the requester core: fq_args[4], fq_request() */
#include "filereq_core.h"

/**
 * @brief FileRequest$(Title$,Pathname$,Filename$): the file requester.
 *
 * Opens the requester (shared/filereq/filereq_core.c) on the used screen.
 * OK (button, double click on a file, Return in the file gadget) copies
 * the path and the file name into path$ and file$ and returns path + "/"
 * (unless it ends in ":" or "/") + file; CANCEL and the close gadget
 * return "".  The result is copied to a3, the place of the title.
 * BUG(orig) (#153): path$/file$ get the new contents but not their new
 * lengths.  BUG(orig) (#161): a3 is left behind the terminating 0.
 * The other BUG(orig) of the requester (#154, #156-#160, #345): see the
 * head comment.  DIFF: if chip memory, a library or the window cannot be
 * opened, the result is "" (the original returns an uninitialized long).
 * Errors (debugger, freqchk): path$ or file$ buffer 0 (error 1), "Not
 * enough space in PATH string" (less than 160 bytes), "Not enough space
 * in NAME string" (less than 64 bytes).
 *
 * @param title  d0: Title$ (a3 points at it, the result goes there)
 * @param path  d1: the buffer of the variable Pathname$ (160 bytes)
 * @param file  d2: the buffer of the variable Filename$ (64 bytes)
 * @param scr  a1: the used screen (!libs #screenslib,$1102)
 * @return d0: the length of the result; d1/a1 kept
 */
static ULONG BLITZ_ROUTINE dofreq_c(ULONG title BL_REG(d0), ULONG path BL_REG(d1),
                                    ULONG file BL_REG(d2), ULONG *scr BL_REG(a1))
{
    UBYTE *s;
    ULONG n;

    fq_args[2] = title;
    fq_args[0] = path;
    fq_args[1] = file;
    fq_args[3] = *scr;
    blitz_string = (UBYTE *)title;
    s = (UBYTE *)fq_request();
    if (!s) {
        *blitz_string = 0;
        return 0;
    }
    n = (ULONG)-1;
    do
        n++;
    while ((*blitz_string++ = *s++) != 0);
    return n;                           /* a3 behind the 0 */
}
BLITZ_THUNK(dofreq, dofreq_c, "d1/a1");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess2, "Not enough space in PATH string");
BLITZ_ERRCHK_MESSAGE(mess3, "Not enough space in NAME string");

static void BLITZ_ERRCHK freqchk_c(ULONG path BL_REG(d1), ULONG file BL_REG(d2))
{
    if (!path || !file)
        BLITZ_RUNERR((const char *)1);
    else if (*(const ULONG *)(path - 8) < 160)
        BLITZ_RUNERR(mess2);
    else if (*(const ULONG *)(file - 8) < 64)
        BLITZ_RUNERR(mess3);
}

BLITZ_ERRCHK_THUNK(freqlib_freqchk, freqchk_c, "d0-d1/a0-a1");

