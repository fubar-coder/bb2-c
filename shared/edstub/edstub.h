/*
 * edstub.h - frame of a headless editor that blitz2 loads as blitz2:ted
 * (from bb2 compilerlauf/tedstub).  A program on this frame gets the
 * compiler's edstruct (token list, xtra hooks, menu callback) and runs its
 * own main instead of an editor; when that returns, blitz2 quits.
 *
 * Interface (ted.s 1.1, blitz2.s 1.95): blitz2 LoadSeg()s blitz2:ted and
 * calls hunk+4 (`outside`, start.c) with a1 = edstruct; it gets a0 = main
 * entry, a1 = comdata (the table of editor services the compiler calls,
 * edstub.c).  Then it calls main (a0 = its argument, the file name).
 *
 * The frame answers the compiler's services without a screen: requesters
 * are remembered but never shown, getinput answers the known ones (the
 * version requester, "Not Enough Object Memory"), DOS requesters are off.
 *
 * Built without any C runtime (-nostdlib, -ffreestanding): no startup
 * code, no libc, no libgcc - no 32 bit multiplication or division.  A
 * program on TED's core (ab3cli) compiles the frame itself with TED's
 * -fbaserel (the entries load its a4, start.c) and EDSTUB_INIT.
 *
 * Runs on Kickstart 1.2 (V33) and later: only functions of V33 (no
 * CreateMsgPort, CreateIORequest, AllocDosObject ...; the helpers below
 * do what those do).
 */
#ifndef EDSTUB_H
#define EDSTUB_H

#include <stddef.h>
#include <exec/types.h>

/**
 * @brief The register frame of the stubs in start.c (movem d0-d7/a0-a6).
 */
struct regs {
    ULONG d[8];
    ULONG a[7];
};

/**
 * @brief The compiler's edstruct (blitz2.s 274; ted.s `outside` copies its
 *        14 words).
 */
struct edstruct {
    APTR tokens;                /* first node: next.l, word.w, name */
    APTR menus;
    APTR loadxtra;              /* d7 = file handle, a6 = DOSBase */
    APTR savextra;
    APTR menu;                  /* menu callback: d6 = item */
    APTR cleanup;
    UBYTE colon, quote, label, remark;
};

/** @brief The edstruct blitz2 passed to `outside` (a1). */
extern struct edstruct *edstub_ed;

/**
 * @brief The program part, provided by the program on this frame.
 *
 * Called by the main entry; when it returns, the frame closes everything
 * and blitz2 quits.
 *
 * @param arg  the argument blitz2 got (a0 of the main entry: the file name)
 */
void edstub_main(const char *arg);

#ifdef EDSTUB_INIT
/**
 * @brief The program's set-up, provided by a program whose frame is
 *        compiled with EDSTUB_INIT defined (ab3cli).
 *
 * Called by the init entry once the frame is ready, before the compiler
 * goes on (loads its libraries, opens the version requester): the place to
 * set the hooks and edstub_quiet for what comes before edstub_main.
 */
void edstub_init(void);
#endif

/**
 * @brief Call code that may change every register (compiler callbacks, TED
 *        routines).
 *
 * Loads d0-d7/a0-a6 from *r, calls fn and stores all registers back into
 * *r.
 *
 * @param fn  the routine to call
 * @param r   the registers: in before the call, out after it
 * @return d0 after the call
 */
ULONG edstub_callregs(APTR fn, struct regs *r);

/**
 * @brief Back to the compiler as if the editor had quit.
 *
 * Drops everything below the main entry's frame (the stack pointer kept
 * by the main entry).  Only inside main.
 */
void edstub_abort(void) __attribute__((noreturn));

/**
 * @name Hooks of the program
 * NULL: the frame's default.
 * @{
 */
/** @brief ezfreq: the file name for the requester with this title, or NULL
 *         (default: cancelled). */
extern const char *(*edstub_ezfreq_hook)(const char *title);
/** @brief The compiler's error message (default: logged). */
extern void (*edstub_error_hook)(const char *msg);
/** @brief tokone: tokenise the line (default: logged, line left as is). */
extern void (*edstub_tokone_hook)(UBYTE *line);
/** @brief Before the frame aborts inside main (compiler waits for input
 *         forever, extalloc failed): the program can log its result and
 *         free its data. */
extern void (*edstub_abort_hook)(const char *why);
/** @brief openio/closeio (ted.s: the console window of a program run from
 *         the editor): called in the program's process, runtheprog, before
 *         and after the program. */
extern void (*edstub_openio_hook)(void);
/** @copydoc edstub_openio_hook */
extern void (*edstub_closeio_hook)(void);
/** @brief getinput2 (the compiler polls the stop requester, chkrealstop):
 *         the id of the gadget hit, 0 = nothing (default: nothing). */
extern UWORD (*edstub_getinput2_hook)(void);
/** @} */

/**
 * @brief The number of requesters open now (openreq not yet closed).
 * @return the depth of the requester stack
 */
int edstub_open_requests(void);

struct MsgPort;
/**
 * @brief The UserPort of the requester window.
 *
 * Its signal is the stop requester's exception (blitz2 openstopreq), a
 * message there is "a gadget was hit".
 *
 * @return the port
 */
struct MsgPort *edstub_req_port(void);

/** @brief Length of edstub_logname. */
#define EDSTUB_PATHLEN 256
/** @brief The log file's name; empty while there is none. */
extern char edstub_logname[EDSTUB_PATHLEN];

/**
 * @brief Log a line: to Output() and appended to the log file.
 *
 * Before there is a log file, the lines are kept and edstub_set_log()
 * writes them first.  The file is opened and closed for every line, so the
 * log survives a crash of the compiler.
 *
 * @param s  the line, without its line feed
 */
void edstub_log(const char *s);

/** @brief Non-zero: edstub_log does not write to Output() (a program with a
 *         console output of its own; the log file still gets the lines). */
extern int edstub_quiet;

/**
 * @brief Set the log file: create it (a new file) and write the lines kept
 *        so far.
 * @param path  the file name (cut to EDSTUB_PATHLEN - 1 characters)
 * @return 1, or 0 if the file cannot be created (no log file then)
 */
int edstub_set_log(const char *path);

/**
 * @brief The busy pointer: sleepon sets it (1), pointeron clears it (ted.s:
 *        the window pointer); the frame shows nothing, it only keeps the
 *        state.
 */
extern int edstub_busy;

/** @brief A new job: the counters of the requester answers start again. */
void edstub_reset_requests(void);

/**
 * @name The racing stripe (#980)
 * The compiler's progress bar.  The original compiler XORs it into the
 * editor's bitmap (comdata+36, rows 0 to 9, comdata+92 bytes per row);
 * blitz2-c reports its cells through TED-C's progress call (comdata+98
 * marker 'TEDp', +102 entry: d0 bar, d1 byte column).  By default the frame
 * has a bitmap of 80 bytes per row and does not announce the call.
 * @{
 */
/**
 * @brief Give the compiler a bitmap of bmapmul bytes per row (comdata+92)
 *        with guard bytes before and after it, and record the stripe from
 *        now on (edstub_stripe_log).
 * @param bmapmul  bytes per row (at least 80)
 * @param guard    guard bytes before and after the bitmap
 * @return 1, or 0 if out of memory (the bitmap stays as it was)
 */
int edstub_set_bitmap(UWORD bmapmul, ULONG guard);

/**
 * @brief Announce the progress call (comdata+98 = 'TEDp') or not (0).
 *
 * A call toggles the cell as the original compiler XORs it (the byte
 * column of rows bar and 9 - bar of the bitmap) and is counted.
 */
void edstub_set_progress(int on);

/**
 * @brief After a compile: log the stripe, then clear the bitmap, refill
 *        the guards and the counters.
 *
 * Lines (tab separated, with the job's id): `PROGRESS ID none` (no call)
 * or per bar with calls `PROGRESS ID bar N COUNTS` (the calls per column
 * from column 0, a digit each, '+' above 9, up to the last column called),
 * plus `PROGRESS ID bad N` for calls with a bar above 3 or a column
 * outside the row; `STRIPE ID none` (bitmap empty) or per row with set
 * bytes `STRIPE ID row R PICTURE` ('#' $FF, '.' 0, 'x' else, up to the
 * last byte not 0, columns 0 to 255); `GUARD ID ok` or `GUARD ID before N
 * after M` (bytes of the guards changed).  The picture and the guards are
 * those of the checkpoints openreq, closereq and the end of the job: the
 * fullest picture, the most changed guard bytes (with two passes the XORs
 * of the second undo those of the first; the requester of the second pass
 * opens between them).  Nothing without edstub_set_bitmap.
 *
 * @param id  the job's id
 */
void edstub_stripe_log(const char *id);
/** @} */

struct IORequest;
/**
 * @brief A message port with a signal of its own, with Kickstart 1.x calls
 *        (CreateMsgPort of V36).
 * @return the port, or NULL if out of memory or signals
 */
struct MsgPort *edstub_create_port(void);

/**
 * @brief Free a port of edstub_create_port() and its signal.
 * @param port  the port (NULL: nothing)
 */
void edstub_delete_port(struct MsgPort *port);

/**
 * @brief An IO request for a port, with Kickstart 1.x calls
 *        (CreateIORequest of V36).
 * @param port  the reply port (NULL: fails)
 * @param size  the size of the request in bytes (cleared)
 * @return the request, or NULL if port is NULL or out of memory
 */
struct IORequest *edstub_create_ioreq(struct MsgPort *port, ULONG size);

/**
 * @brief Free a request of edstub_create_ioreq() (as DeleteIORequest: the
 *        node's successor and io_Device set to -1 first).
 * @param io  the request (NULL: nothing)
 */
void edstub_delete_ioreq(struct IORequest *io);

/**
 * @name Small libc replacements
 * memset/memcpy for the compiler as well.
 * @{
 */
void *memset(void *d, int c, size_t n);
void *memcpy(void *d, const void *s, size_t n);
/** @brief The length of the string s. */
ULONG edstub_strlen(const char *s);
/** @brief 1 if the strings are equal, else 0. */
int edstub_streq(const char *a, const char *b);
/** @brief 1 if s starts with pre, else 0. */
int edstub_prefix(const char *s, const char *pre);
/** @brief Copy s to d, at most max - 1 characters and the 0 (nothing if
 *         max is 0). */
void edstub_strcpy(char *d, const char *s, ULONG max);
/** @brief Append s to d; max is the size of all of d (as edstub_strcpy). */
void edstub_strcat(char *d, const char *s, ULONG max);
/** @brief u / 10 without libgcc (no __udivsi3 in a -nostdlib build); the
 *         remainder to *rem. */
ULONG edstub_udiv10(ULONG u, ULONG *rem);
/** @} */

/**
 * @brief A bounded string builder: buf of max bytes, len used, always
 *        0-terminated (what does not fit is dropped).
 */
struct sb {
    char *buf;
    ULONG len, max;
};

/** @brief Append the character c. */
void sbc(struct sb *b, char c);
/** @brief Append the string s ("(null)" for NULL). */
void sbs(struct sb *b, const char *s);
/** @brief Append v in decimal. */
void sbn(struct sb *b, LONG v);
/** @brief Append the low digits hex digits of v, upper case. */
void sbx(struct sb *b, ULONG v, int digits);
/** @brief Append s quoted, controls (< 32 and 127) as \\xNN. */
void sbq(struct sb *b, const char *s);

#endif
