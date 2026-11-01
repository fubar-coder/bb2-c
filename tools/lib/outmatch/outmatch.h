/*
 * outmatch.h - the output of a test run against an expect file, line by
 * line (the emulator tests of blitz2-ab3, tests/ab3).
 *
 * The expect file is a list of lines; each one stands for output lines:
 *   TEXT        one line equal to TEXT
 *   ~PATTERN    one line that matches PATTERN
 *   +PATTERN    one or more lines that match PATTERN: as many as match
 *               (greedy, no backtracking)
 *   \TEXT       one line equal to TEXT (for a TEXT that starts with '~',
 *               '+' or '\')
 * In a PATTERN, '*' matches any characters (also none), '#' one or more
 * decimal digits with an optional '-' before them, "\c" the character c;
 * every other character itself.  The output must have no further lines.
 *
 * Lines end with LF; a CR before it and a missing LF at the end of the
 * text are ignored (an empty last line is no line).
 *
 * Sections: a line "[NAME]" (a '[', letters, digits, '-' or '_', a ']')
 * starts the section NAME, which goes up to the next such line.  With a
 * section given, only that section of the expect file and of the output
 * are compared (both must have it).
 */
#ifndef OUTMATCH_H
#define OUTMATCH_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Whether a line matches a pattern (the PATTERN syntax above).
 * @param pat   the pattern (0-terminated)
 * @param line  the line, line[0..len)
 * @param len   its length
 * @return 1 or 0
 */
int outmatch_line(const char *pat, const char *line, size_t len);

/**
 * @brief Compare an output with an expect file.
 *
 * @param expect   the expect file's text (0-terminated)
 * @param output   the output's text (0-terminated)
 * @param section  NULL: the whole texts; else the name of the section to
 *                 compare (without the brackets)
 * @param msg      the first difference, one line without LF ("expect
 *                 line 3: \"x\", output line 3: \"y\"", "missing section
 *                 [run] in the output", ...); "" on a match
 * @param cap      the size of msg in bytes (> 0)
 * @return 0 = the output matches, 1 = it does not (msg), -1 = the expect
 *         file is broken (msg: a '~' or '+' without a pattern, its
 *         section missing)
 */
int outmatch(const char *expect, const char *output, const char *section, char *msg, size_t cap);

#ifdef __cplusplus
}
#endif

#endif
