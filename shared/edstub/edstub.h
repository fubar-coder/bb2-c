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

/* log: a line to Output() and, if edstub_logname[0], appended to that file */
#define EDSTUB_PATHLEN 256
extern char edstub_logname[EDSTUB_PATHLEN];
void edstub_log(const char *s);

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
