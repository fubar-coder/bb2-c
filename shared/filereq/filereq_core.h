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
 * Opens dos, intuition and graphics at every call and never closes them
 * (BUG(orig) #154); the other kept original bugs (#156-#160, #345) are
 * listed in filereq_core.c.
 *
 * @return the address of the result "path/file" (inside the data block),
 *         or 0 (cancel, close gadget, no memory/window; DIFF: the original
 *         returns an uninitialized long when memory, a library or the
 *         window cannot be opened)
 */
ULONG fq_request(void);

#endif
