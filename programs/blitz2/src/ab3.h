/*
 * ab3.h - the state shared by the AB3 modules of blitz2-ab3 (ab3*.c, the
 * implementation of the extension points of ext.h; docs/ab3/variant.md).
 * Not for the chunk files: they only know ext.h.
 */
#ifndef AB3_H
#define AB3_H

#include "xtra.h"
#include "ab3job.h"

/** @brief The job of a command line run (ab3main.c); NULL in an editor
 *         session. */
extern struct ab3job *ab3job;

/** @brief The mode of the program being compiled (enum xtra_mode,
 *         shared/xtra): XTRA_MODE_BB2 or XTRA_MODE_AB3.  One mode per
 *         compile, decided by the command line (ab3main.c) before anything
 *         is loaded; an editor session is always BB2.  (ab3tok.c) */
extern int progmode;

/** @brief Set the mode of the program (progmode) and the data of the mode
 *         of ext.h (ext_toklist, ext_mathq) - once, before anything is
 *         loaded. */
void ab3_setmode(int mode);

/** @brief The error of a missing library file ("Library file not found:
 *         NAME"), NULL if none (ab3libs.c): makelibs finds it at start-up,
 *         where there is no compile to stop yet; compile reports it. */
extern const char *ab3liberr;

/** @brief A pass starts: the state of the syntax rules (ab3syn.c) goes
 *         back to its start (called by ext_passstart, ab3dir.c). */
void ab3_synpass(void);

/** @brief A pass starts: no sections (ab3sect.c, called by ext_passstart). */
void ab3_sectpass(void);

/** @brief The directives BSS and Chip (ab3sect.c): a section starts. */
void ab3_bss(void);
void ab3_chip(void);

/** @brief The Version (major, minor, patch) and NumberOfBuilds of the
 *         main source's embedded header, 0 without (ab3main.c): the values
 *         of the escapes \\__VER_* (ab3str.c). */
extern UBYTE ab3version[3];
extern ULONG ab3builds;

/** @brief The include depth: the includes open now (0 in the main
 *         source, 1 inside an include, more in nested ones), for
 *         AmiBlitz3's constant #__include (ab3inc.c). */
UWORD ab3_incdepth(void);

/* ---- the line counter and the current include (change ab3-mode-g3, D5) */

/** @brief The empty lines the main source's embedded header became
 *         (ab3cli keeps them, AmiBlitz3 has no such lines), 0 without
 *         (ab3main.c): \\__CODELINE does not count them. */
extern ULONG ab3hdrlines;

/** @brief Is the line just read one of the empty lines the embedded
 *         header of the innermost include became (ab3inc.c)? */
BOOL ab3_incheaderline(void);

/** @brief The file name, without its path, of the innermost open include,
 *         "" in the main source (ab3inc.c): \\__THIS_INCLUDE. */
const char *ab3_incfile(void);

/** @brief The next line of the program is read (ext_nextline, ab3syn.c;
 *         also each line it joins with @@): \\__CODELINE counts it
 *         (ab3str.c). */
void ab3_strline(void);

/** @brief The lines between the one of the joined line (@@) that p is in
 *         and its last line; 0 if p is not in the joined line being
 *         compiled (ab3syn.c): \\__CODELINE of a string in an earlier
 *         line of it. */
ULONG ab3_joinback(const UBYTE *p);

/** @brief A pass starts: the line counter of \\__CODELINE back to 0
 *         (ab3str.c, called by ext_passstart). */
void ab3_strpass(void);

#endif
