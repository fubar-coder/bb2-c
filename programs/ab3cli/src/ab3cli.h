/*
 * ab3cli.h - the parts of ab3cli (main.c) the core's hooks (hooks.c) call.
 */
#ifndef AB3CLI_H
#define AB3CLI_H

/**
 * @brief The job cannot go on: print "FILE: why" (FILE = the file being
 *        worked on), return code 20, back to the compiler (edstub_abort).
 * @param why  the reason
 */
void ab3cli_fatal(const char *why) __attribute__((noreturn));

/**
 * @brief TED's error requester while a file is loaded (error1/error2 of
 *        the core): print "FILE: msg" and remember that loading failed.
 * @param msg  the message
 */
void ab3cli_loaderror(const char *msg);

#endif
