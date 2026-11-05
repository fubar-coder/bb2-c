/*
 * cov.h - function coverage of an m68k program built with
 * -finstrument-functions-once (the coverage configuration, preset
 * amiga-cov; openspec change test-coverage, D2 and D3).
 *
 * The compiler calls __cyg_profile_func_enter at the first entry of every
 * instrumented function; the hook sets the function's bit in a bitmap over
 * the code hunk (one bit per 2-byte offset, covfmt.h).  At the program's
 * end the bitmap is OR-ed into TEST:cov/<program>.cov.
 *
 * With libnix's startup code nothing has to be called: cov.c puts
 * cov_start into libnix's constructor list and cov_write into its
 * destructor list, which libnix runs when main returns and on exit().
 *
 * cov.c is compiled with the program's own options (-fbaserel) but not
 * instrumented, and so are the entry stubs that load a4 (glue*.c): the hook
 * addresses its globals relative to a4.  COV_PROGRAM (a string) names the
 * file, e.g. "blitz2" for TEST:cov/blitz2.cov.
 */
#ifndef SHARED_COV_COV_H
#define SHARED_COV_COV_H

/* allocate the bitmap (the size of the code hunk); before it, the hook
   records nothing */
void cov_start(void);

/* OR the bitmap into TEST:cov/<COV_PROGRAM>.cov and free it.  Without
   TEST:cov nothing is written and no requester shows; errors are ignored. */
void cov_write(void);

/* the hooks of -finstrument-functions(-once) */
void __cyg_profile_func_enter(void *fn, void *site);
void __cyg_profile_func_exit(void *fn, void *site);

#endif
