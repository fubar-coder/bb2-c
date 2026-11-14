/*
 * filereq_core.h - the file requester of Blitz Basic 2 (filereq_core.c),
 * shared by freqlib (libs-c/libs/freqlib.c) and TED (ted-c/src/filereq.c).
 * Each program compiles filereq_core.c itself and provides filereq_env.h.
 */
#ifndef FILEREQ_CORE_H
#define FILEREQ_CORE_H

#include <exec/types.h>

/**
 * @brief The parameters of fq_request(), set before the call.
 *
 * The same order as the 4-long `reqstruct` of TED and the parameter block
 * of freqlib:
 *   - [0] path buffer (160 bytes, in/out: the start path; on OK the path
 *     chosen)
 *   - [1] file buffer (64 bytes, in/out: the start file; on OK the file
 *     chosen)
 *   - [2] window title
 *   - [3] screen (0 = Workbench)
 */
extern ULONG fq_args[4];

/**
 * @brief Run the file requester with the parameters in fq_args.
 *
 * Original entry: code+4 ($CA in freqlib), the parameter block in a0.
 * Opens dos, intuition and graphics at every call and closes them at its
 * end (fixed #154); the other fixed original bugs are listed in
 * filereq_core.c.
 *
 * @return the address of the result "path/file" (inside the data block),
 *         or 0 (cancel, close gadget, no memory/window; DIFF: the original
 *         returns an uninitialized long when memory, a library or the
 *         window cannot be opened)
 */
ULONG fq_request(void);

/**
 * @brief The end of the program: undo the requester's CD (fixed #157).
 *
 * The gadget "CD" makes the path the current directory of the process and
 * keeps the directory before the first CD.  fq_finit makes that directory
 * current again and unlocks the requester's own lock (not a lock the
 * program made current itself since); nothing when CD was not used.
 * Called once at the end: freqlib's finit, the end of TED.  Opens and
 * closes dos.library itself.
 */
void fq_finit(void);

#endif
