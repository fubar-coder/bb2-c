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
 * code, no libc, no libgcc - no 32 bit multiplication or division.
 *
 * Runs on Kickstart 1.2 (V33) and later: only functions of V33 (no
 * CreateMsgPort, CreateIORequest, AllocDosObject ...; the helpers below
 * do what those do).
 */
#ifndef EDSTUB_H
#define EDSTUB_H

#include <stddef.h>
#include <exec/types.h>

/* register frame of the stubs in start.c (movem d0-d7/a0-a6) */
struct regs {
    ULONG d[8];
    ULONG a[7];
};

/* edstruct (blitz2.s 274; ted.s `outside` copies its 14 words) */
struct edstruct {
    APTR tokens;                /* first node: next.l, word.w, name */
    APTR menus;
    APTR loadxtra;              /* d7 = file handle, a6 = DOSBase */
    APTR savextra;
    APTR menu;                  /* menu callback: d6 = item */
    APTR cleanup;
    UBYTE colon, quote, label, remark;
};

extern struct edstruct *edstub_ed;

/* the program part: called by main with the argument blitz2 got */
void edstub_main(const char *arg);

/* load registers from *r, call fn, store all registers back into *r */
ULONG edstub_callregs(APTR fn, struct regs *r);

/* back to the compiler as if the editor had quit (only inside main) */
void edstub_abort(void) __attribute__((noreturn));

/* hooks of the program (NULL: default) */
extern const char *(*edstub_ezfreq_hook)(const char *title);  /* file name or NULL */
extern void (*edstub_error_hook)(const char *msg);
extern void (*edstub_tokone_hook)(UBYTE *line);
/* before the frame aborts inside main (compiler waits for input forever,
   extalloc failed): the program can log its result and free its data */
extern void (*edstub_abort_hook)(const char *why);
/* openio/closeio (ted.s: the console window of a program run from the
   editor): called in the program's process, runtheprog, before and after
   the program */
extern void (*edstub_openio_hook)(void);
extern void (*edstub_closeio_hook)(void);
/* getinput2 (the compiler polls the stop requester, chkrealstop): the id
   of the gadget hit, 0 = nothing (default: nothing) */
extern UWORD (*edstub_getinput2_hook)(void);

/* the requesters open now (openreq not yet closed) */
int edstub_open_requests(void);
/* the UserPort of the requester window: its signal is the stop
   requester's exception (blitz2 openstopreq), a message there is "a
   gadget was hit" */
struct MsgPort;
struct MsgPort *edstub_req_port(void);

/* log: a line to Output() and appended to the log file; before there is
   one, the lines are kept and edstub_set_log writes them first (new file;
   0 = cannot create it, no log file then) */
#define EDSTUB_PATHLEN 256
extern char edstub_logname[EDSTUB_PATHLEN];
void edstub_log(const char *s);
int edstub_set_log(const char *path);

/* the busy pointer: sleepon sets it, pointeron clears it (ted.s: the
   window pointer); the frame shows nothing, it only keeps the state */
extern int edstub_busy;

/* a new job: the counters of the requester answers start again */
void edstub_reset_requests(void);

/* a message port with a signal of its own (CreateMsgPort of V36) and an
   IO request of size bytes for port (CreateIORequest of V36), with
   Kickstart 1.x calls; NULL if out of memory or signals */
struct IORequest;
struct MsgPort *edstub_create_port(void);
void edstub_delete_port(struct MsgPort *port);
struct IORequest *edstub_create_ioreq(struct MsgPort *port, ULONG size);
void edstub_delete_ioreq(struct IORequest *io);

/* small libc replacements (memset/memcpy for the compiler as well) */
void *memset(void *d, int c, size_t n);
void *memcpy(void *d, const void *s, size_t n);
ULONG edstub_strlen(const char *s);
int edstub_streq(const char *a, const char *b);
int edstub_prefix(const char *s, const char *pre);
void edstub_strcpy(char *d, const char *s, ULONG max);
void edstub_strcat(char *d, const char *s, ULONG max);
ULONG edstub_udiv10(ULONG u, ULONG *rem);

/* bounded string builder */
struct sb {
    char *buf;
    ULONG len, max;
};

void sbc(struct sb *b, char c);
void sbs(struct sb *b, const char *s);
void sbn(struct sb *b, LONG v);
void sbx(struct sb *b, ULONG v, int digits);    /* upper case hex */
void sbq(struct sb *b, const char *s);          /* quoted, controls as \xNN */

#endif
