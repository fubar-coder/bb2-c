/*
 * warnlist.h - the compiler's warnings (spec compiler/warnings): one line
 * per warning, its identifier and its name, in the order of their numbers
 * (bit n of a warning set).  The one list of the warnings for every side
 * that knows them: the compiler core (parse.h W_*, the names of the
 * editor's messages), the job and the command line (ab3job.h
 * AB3JOB_W_*, ab3args_warnings).  Macros only, no types: each side
 * includes it in its own world.
 *
 * X(ID, NAME): ID the identifier part (W_ID, AB3JOB_W_ID), NAME the
 * warning's name as a string (-w, -werr, the console's "[NAME]").
 */
#ifndef WARNLIST_H
#define WARNLIST_H

#define WARNLIST(X)                                                          \
    X(RESCLASH, "resclash")     /* a constant of the residents with two    \
                                   values (stmts.c checkconstclash) */       \
    X(DIVFRAC, "divfrac")       /* the whole-division exception */           \
    X(DIVQUICK, "divquick")     /* a whole division beside a Quick */        \
    X(FLOATTOP, "floattop")     /* the top operation under a Float */        \
    X(SHIFTFRAC, "shiftfrac")   /* the shift exception */                    \
    X(SHIFTWRAP, "shiftwrap")   /* the left-shift wrap */                    \
    X(LONGWRAP, "longwrap")     /* a Float beyond the Long range wraps,     \
                                   known at compile time */                  \
    X(DATAFRAC, "datafrac")     /* a Data.b value with a fraction */

#endif
