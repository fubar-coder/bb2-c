/*
 * ext.h - the extension points of the compiler's core (design D1/D2 of
 * the change blitz2-ab3-extension-points).
 *
 * The chunk files (the translation of blitz2.s) are the same for blitz2
 * and blitz2-ab3.  Where blitz2-ab3's AB3 mode differs, they call one of
 * these functions or read one of these objects; each program links one
 * implementation:
 *
 *   blitz2      ext_bb2.c  BB2 only: nothing handled, BB2's data
 *   blitz2-ab3  ab3*.c     BB2 and AB3 mode (docs/ab3/variant.md)
 *
 * A point that stands for a part of a chunk function returns whether it
 * handled the case (TRUE: the chunk file skips its own code, which is
 * blitz2's); the other points are notifications.  The BB2 behaviour stays
 * in the chunk files: an implementation that handles nothing gives
 * blitz2's results.
 */
#ifndef EXT_H
#define EXT_H

#include <exec/types.h>
#include <dos/dos.h>

struct node;
struct token;
struct include;
struct resentry;

/* ================================================ the data of the mode */

/** @brief The first node of the token list of the program's mode (asm
 *         `tokens`, main.h); blitz2's list (tokimg) in BB2 mode. */
extern struct token *ext_toklist;

/** @brief The library of the float power (eval.c dopow, asm `mathq`);
 *         247-55 in BB2 mode. */
extern UWORD ext_mathq;

/* ======================================================== libraries */

/**
 * @brief makelibs (main.c): find the library files of the mode.
 * @param ok  makelibs' result if handled (FALSE: error)
 * @return TRUE if handled: deflibs, acidlibs and libsdir are not used
 */
BOOL ext_makelibs(BOOL *ok);

/**
 * @brief loadlibs (asmlibs.c): load the library files found by
 *        ext_makelibs, linked in behind *tail.
 * @return TRUE if handled: acidlibs and deflibs are not loaded
 */
BOOL ext_loadlibs(struct node **tail);

/**
 * @brief loadblitzlibs (asmlibs.c): load the userlibs of the mode behind
 *        tail and end their list; fixlibs follows.
 * @return TRUE if handled: the directories of libslist are not read
 */
BOOL ext_loaduserlibs(struct node *tail);

/** @brief freelibs (asmlibs.c): free what ext_loadlibs loaded. */
void ext_freelibs(void);

/* ======================================================== residents */

/** @brief loadres (stmts.c) starts loading resbuff's residents. */
void ext_resbegin(void);

/**
 * @brief loadres (stmts.c): load the resident res (a name given).
 * @return TRUE if handled: res->seg is set (0: not loaded, the next one
 *         follows - no reserr)
 */
BOOL ext_loadres(struct resentry *res);

/* ========================================================== compile */

/** @brief compile (compile.c) starts the passes: may stop it with err(). */
void ext_compilestart(void);

/* ========================================================= includes */

/** @brief doinc2 (tokens1.c) opened the include inc (its name in
 *         namebuff); may stop the compile with err(). */
void ext_incopen(struct include *inc);

/**
 * @brief readinc (libcall.c): read up to len bytes of inc into dst.
 * @param got  the bytes read if handled (-1: error)
 * @return TRUE if handled: the file is not read
 */
BOOL ext_readinc(struct include *inc, UBYTE *dst, LONG len, LONG *got);

/** @brief readinc took the next line of inc. */
void ext_incline(struct include *inc);

/** @brief The include inc is closed or freed (closeinc, freeincs). */
void ext_incdone(struct include *inc);

/* =================================================== errors, tokens */

/** @brief err (parse.c): a compile error, before the editor is told. */
void ext_errpos(void);

/** @brief A built-in token tok (without bit 15) is compiled as a
 *         statement or a function; may stop the compile with err(). */
void ext_newtoken(UWORD tok);

/* ============================ AB3 mode's language (change ab3-mode-g2) */

/** @brief dopass (compile.c) starts a pass, its state cleared
 *         (freeemup): the state of the directives goes back to its
 *         start. */
void ext_passstart(void);

/**
 * @brief handletoken (compile.c): the token c (bit 15 set) starts a
 *        statement.
 * @return TRUE if handled (the statement is compiled, the next character
 *         fetched): blitz2's handler is not called
 */
BOOL ext_token(UWORD c);

/** @brief Is the "new syntax" of AB3 mode on (OPTIMIZE bit 2)?  Always
 *         FALSE in BB2 mode. */
BOOL ext_newsyntax(void);

/* ======================================= the command line, the editor */

/**
 * @brief main: the program's arguments, before anything is loaded.
 * @param argc  main's argc (0: started from the Workbench)
 * @param rc    the return code of the program if handled
 * @return TRUE if handled: the program ends with *rc
 */
BOOL ext_cmdline(int argc, char **argv, int *rc);

/** @brief begin (main.c): the editor to load instead of name (blitz2:ted). */
const char *ext_editor(const char *name);

/** @brief begin: the editor's argument (a0) instead of arg (the file to
 *         edit). */
APTR ext_editorarg(APTR arg);

/**
 * @brief main: the compiler is done (begin returned).
 * @param rc  the return code of the program if handled
 * @return TRUE if handled, else main returns 0 (BUG(orig) #22)
 */
BOOL ext_result(int *rc);

#endif
