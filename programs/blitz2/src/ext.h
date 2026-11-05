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
struct macro;
struct var;
struct proc;

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

/**
 * @brief doinclude (tokens1.c): the name of an INCLUDE is in namebuff
 *        (makeiname, len bytes); may replace it by the file to include and
 *        stop the compile with err().
 * @return the length of the name in namebuff now (len if unchanged)
 */
UWORD ext_incname(UWORD len);

/**
 * @brief doxinclude (tokens1.c): an XINCLUDE of the name in namebuff
 *        (makeiname, len bytes).
 * @return TRUE if handled: skipped, or included (then it does not return:
 *         doinc2); FALSE: blitz2's way (the whole name)
 */
BOOL ext_xinclude(UWORD len);

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

/* ==================== a Function as a statement (ab3-function-statement) */

/**
 * @brief handleproc (compile.c): the procedure p called at the start of a
 *        statement (name{...}) is a Function (it has a return type).
 * @return TRUE if allowed: the call is compiled as for a Statement, the
 *         result discarded (blitz2: "Illegal Procedure Call")
 */
BOOL ext_funcstmt(const struct proc *p);

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

/**
 * @brief expandmax (compile.c): the parameters of the call of the macro
 *        m, at *pp (behind the name and its spaces).
 * @param narg  their number if handled (in macrobuff as blitz2's)
 * @return TRUE if handled: *pp is behind them (blitz2 takes them only
 *         from braces)
 */
BOOL ext_macargs(struct macro *m, UBYTE **pp, WORD *narg);

/**
 * @brief handleascii (compile.c): a name at the start of a statement is
 *        followed by c (not the end, not '{').
 * @return TRUE if it is a label (the source is back at c, the statement
 *         ends there): blitz2 compiles an assignment
 */
BOOL ext_namelabel(UWORD c);

/** @brief handlemouse (compile.c): the label .name is followed by c; may
 *         end it before c (the source back at c, the statement ends
 *         there). */
void ext_dotlabel(UWORD c);

/** @brief handlechar (compile.c): the first character c of a statement
 *         (not its end); returns the one to compile it from. */
UWORD ext_stmtprefix(UWORD c);

/**
 * @brief getcharb (parse.c): a comment starts at srcptr (behind ';').
 * @return TRUE if code follows in it: srcptr is on that code, getcharb
 *         returns ':' (blitz2 skips the rest of the line)
 */
BOOL ext_comment(void);

/** @brief getelement2 (eval.c): does c start a variable (besides
 *         blitz2's letters, '*', '\')? */
BOOL ext_letter(UWORD c);

/** @brief docnif (tokens1.c): the condition of a CNIF starts with c;
 *         returns the character it starts with now (another one if the
 *         mode reads a prefix of the condition). */
UWORD ext_cnifstart(UWORD c);

/** @brief docnif: the condition of the CNIF is evaluated. */
void ext_cnifend(void);

/**
 * @brief fetchconst (eval.c): the constant named namebuff (len bytes)
 *        in an expression.
 * @return TRUE if handled: *value is its value (blitz2 looks for it in
 *         the labels, "Constant Not Found" if none)
 */
BOOL ext_constant(UWORD len, ULONG *value);

/**
 * @brief doshared (tokens1.c): SHARED outside a procedure, c its first
 *        character.
 * @return TRUE if handled (the statement is compiled): blitz2's error
 */
BOOL ext_shared(UWORD c);

/** @brief fvarlocal (parse.c): the variable named namebuff (len, the
 *         kind of flagmask) is not one of the procedure's SHARED ones:
 *         a variable shared with every procedure, NULL if none. */
struct var *ext_sharedvar(UWORD len);

/** @brief nextline (compile.c): srcptr is on the next line of the program
 *         (of the source or an include); may put it on another one (a
 *         line joined with the following ones). */
void ext_nextline(void);

/** @brief dopass (compile.c): the program is compiled (errchx2), its end
 *         code follows; may stop the compile with err(). */
void ext_passend(void);

/**
 * @brief savefile (parse.c): write the executable (execname).
 * @return TRUE if handled: blitz2's hunk, relocations and symbols are not
 *         written (the icon is)
 */
BOOL ext_savefile(void);

/* ======================== AB3 mode's strings (change ab3-mode-g3) */

/**
 * @brief litstring (eval.c): the character read in a string literal is
 *        a '\'; srcptr is behind it.
 * @param len  the length of the string so far; the bytes written are
 *             added if handled
 * @return TRUE if handled: an escape sequence was read and its bytes
 *         written with pokedata1b (the '\' is not written); may stop the
 *         compile with err()
 */
BOOL ext_strescape(UWORD *len);

/* ---- Dc.b strings (ab3-mode-g3 task 3.1) */

/**
 * @brief dodc (tokens1.c): the character read in a string of Dc.b is a
 *        '\'; srcptr is behind it.
 * @return TRUE if handled: an escape sequence was read and its bytes
 *         written with pokebyte (the '\' is not written)
 */
BOOL ext_dcbescape(void);

/* ================ AB3 mode's type rules (change ab3-pointer-assign) */

/** @brief dolet2 (tokens2.c): the right side of an assignment to a
 *         variable that is not a string is evaluated next (eval at
 *         evaldepth + 1, eval.h). */
void ext_assignstart(void);

/**
 * @brief eval.c: an element of an expression gave its value, at
 *        evaldepth: a variable (variable, ampersand2), a function's
 *        result (afunction) or a token (getelement2: a library or
 *        built-in function, code 0).
 * @param code   its type: the type code (1..7, struct type u.code;
 *               >= 256 a NewType), 0 if none
 * @param flags  flagmask of the variable (1: a pointer), 0 if none
 */
void ext_evalvalue(ULONG code, UWORD flags);

/** @brief dolet2: the right side of the assignment ext_assignstart
 *         announced is evaluated; vartype is the left side's (calcvar:
 *         bit 15 its address in a2, bit 14 a pointer).  May stop the
 *         compile with err(). */
void ext_assigncheck(UWORD vartype);

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

/* ============= AB3 mode's character literals (change ab3-mode-g3) */

/**
 * @brief getelement2 (eval.c): an element of a run-time expression
 *        starts with the character c that blitz2 has no element for;
 *        srcptr is behind it.
 * @param type  the type of the expression (asm d2), set if handled
 * @param ret   the next character if handled (as an element returns it)
 * @return TRUE if handled: the element was read and its code made; may
 *         stop the compile with err().  FALSE: blitz2's syntaxerr
 */
BOOL ext_element(UWORD c, UWORD *type, UWORD *ret);

/* ======================== AB3 mode's macros (change ab3-mode-g3) */

/** @brief domacro (tokens1.c): the lines read from here on belong to a
 *         Macro definition (on TRUE, behind the name of its Macro line),
 *         up to its End Macro line (FALSE, when the definition is
 *         stored). */
void ext_macrodef(BOOL on);

/** @brief domacro: the token c is stored in the macro text; next is the
 *         token or character read behind it if c is End ($8002, read
 *         with getchar3), else 0. */
void ext_macrotoken(UWORD c, UWORD next);

/** @brief domacro: a line of the definition ends, and text was stored;
 *         returns how many ": End If" (':', $8002, $800B) are stored
 *         before the ':' of the line end. */
UWORD ext_macrolineend(void);

/** @brief domacro: End ($8002) is stored in the macro text, next is the
 *         character or token read behind it with getchar3 (not End
 *         Macro).
 * @return TRUE if End ends its statement there (AB3 mode: next is 0 -
 *         the line end or a comment - or ':'): domacro backs up
 *         (bakup) and reads next again, so a line end goes through
 *         nextline and ext_macrolineend.  FALSE: blitz2 stores next
 *         as a word (BUG(orig) #968) */
BOOL ext_macroend(UWORD next);

/* ===================== AB3 mode's procedures (change ab3-mode-g4) */

/* ---- the definition (tokens2.c dofunc, dofunc2) */

/**
 * @brief dofunc: the return type of a Function follows (the source is
 *        back at it, as for sizespec2).
 * @param mode  the type code if handled (procmode: 1..7)
 * @return TRUE if handled: the type was read (blitz2: sizespec2, which
 *         refuses a NewType)
 */
BOOL ext_procrettype(UWORD *mode);

/** @brief dofunc2: the name of the procedure follows (prevloc is on its
 *         first character, makename2 backs up to it); may read a word
 *         before it and move prevloc behind it. */
void ext_procfast(void);

/** @brief dofunc2: the procedure p is defined (its node made, nothing of
 *         its parameters read yet). */
void ext_procdef(struct proc *p);

/** @brief dofunc2: a parameter of p, the npar-th (0-based), starts with c;
 *         returns the character its variable starts with (another one if
 *         the mode reads a prefix). */
UWORD ext_procoptional(const struct proc *p, UWORD npar, UWORD c);

/** @brief dofunc2: the number of parameters that gives "Too many
 *         parameters" (blitz2: 7). */
UWORD ext_procmax(void);

/**
 * @brief dofunc2: the parameter npar (0-based, not a string) of type
 *        partype (1..5) is the local at offset off(a4).
 * @return TRUE if handled: its store is written by ext_procentry
 *         (blitz2: move.x dN,off(a4) here)
 */
BOOL ext_procparam(UWORD npar, UWORD partype, WORD off);

/**
 * @brief dofunc2: the parameters of p are read (p->nparams, p->params set);
 *        the string parameters are in st (nstrs pairs of parameter number
 *        and local offset, as stbuff).
 * @return TRUE if handled: the code taking the parameters from the stack
 *         is written (blitz2: the string loop, one long per string)
 */
BOOL ext_procentry(const struct proc *p, const UWORD *st);

/** @brief The bytes of p's parameters on the stack, removed by its
 *         epilogue (endstate) and taken off stackuse after a call
 *         (blitz2: 4 per string parameter). */
UWORD ext_procstackbytes(const struct proc *p);

/** @brief linkput (tokens2.c): are p's locals cleared at its entry (jsr
 *         clrloc behind its link; blitz2: TRUE)? */
BOOL ext_procclrloc(const struct proc *p);

/** @brief procfixer (tokens2.c): p needs local memory (p->nstatics != 0)
 *         and its setup code is called at its entry; may stop the
 *         compile with err(). */
void ext_procstatics(const struct proc *p);

/* ---- the calls (compile.c handleproc, eval.c afunction, asmlibs.c) */

/** @brief afunction: the type code of the Function p's result for
 *         ext_evalvalue (blitz2: its return type). */
ULONG ext_proctype(const struct proc *p);

/**
 * @brief fetchprocps (asmlibs.c): the arguments of a call of p follow
 *        (srcptr behind the '{'; p has parameters).
 * @return TRUE if handled: the call has none, and the code giving every
 *         parameter its value is written; srcptr is still before the '}'
 *         (blitz2: the arguments are fetched)
 */
BOOL ext_procnoargs(const struct proc *p);

/** @brief fetchprocps: the arguments of a call of p are fetched from here
 *         on (p), or they are fetched (NULL, for the innermost one). */
void ext_procargs(const struct proc *p);

/**
 * @brief fetchpees_p (asmlibs.c): the argument list of a call of p ends
 *        after n arguments (fewer than its parameters: no ',' follows).
 * @return TRUE if handled: the code giving the parameters n and up their
 *         value is written (blitz2: "Not Enough Parameters")
 */
BOOL ext_procdefaults(const struct proc *p, UWORD n);

/** @brief sbtolong (eval.c): the code loading the start of the string
 *         into D(regat) is written next to here; may write code that
 *         leaves the registers alone. */
void ext_strpointer(void);

#endif
