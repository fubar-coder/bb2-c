/*
 * datalib - system library 64535 ($CD00): start of the program's DATA
 * statements for Restore/Read (token getdstart, blitz2.s equate 184).
 *
 * C translation of bblibsrc/datalib.  The program start code calls the init
 * routine with the address of the data area in d0; getstart returns it.
 */
#include "blitzlib.h"

/* entry points (thunks / asm, defined below) */
void getstart(void), datalib_codeinit(void);

BLITZ_ENTRY_DECL(init);

/* !syslibheader{#datalib,$cd00,$cd01,init,1,0,0} */
BLITZ_SYSLIBHEADER7(LIB_datalib, 0xcd00, 0xcd01, init, BL_LONGRET, 0, 0);
BLITZ_NULLSUB(getstart, 0, 0);                  /* $CD00 getdstart */
BLITZ_NULLSUB_AT(init, datalib_codeinit, 0, 0); /* init */
BLITZ_LIBFIN();

static ULONG datastart;

/**
 * @brief The init routine: remember the start of the program's DATA area.
 *
 * The program start code calls it with the address of the data area.
 *
 * @param start  d0: the start of the DATA area
 * @return d0: start, unchanged (the init value)
 */
static ULONG BLITZ_ROUTINE codeinit_c(ULONG start BL_REG(d0))
{
    datastart = start;
    return start;
}
BLITZ_THUNK(datalib_codeinit, codeinit_c, "d1/a0-a1");

/**
 * @brief getdstart ($CD00): the start of the program's DATA area.
 *
 * For Restore/Read; keeps d1/a0-a1 (the thunk).
 *
 * @return d0: the address the init routine got
 */
static ULONG BLITZ_ROUTINE getstart_c(void)
{
    return datastart;
}
BLITZ_THUNK(getstart, getstart_c, "d1/a0-a1");
