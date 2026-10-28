/*
 * serial.h - the remote end of the serial tests (bb2 compilerlauf/tools/
 * serialrun.py): Amiberry offers the emulated serial port as a TCP server
 * (serial_port=TCP://127.0.0.1:PORT); a thread connects to it as a client
 * and answers deterministically, only triggered by received bytes, so
 * timing does not matter.  Bytes are compared with bit 7 cleared (works
 * with 7 data bits too):
 *   01 NAME 02   start of a session (NAME = the test)
 *   05 CODE      a canned reply: A "Z", B "HELLO", C "0123456789ABCDEF",
 *                D bytes $80..$87, E echo of the data bytes received since
 *                the previous request, L 100 bytes "0".."W" repeated, W "w"
 *                after 0.1 s, anything else "?"
 *   04           end of the session
 * Per session: everything received raw (with the control bytes) and a
 * readable transcript of both directions.
 */
#ifndef CORPUS_SERIAL_H
#define CORPUS_SERIAL_H

#include <stddef.h>
#include <stdint.h>

struct responder;

/* a free TCP port on 127.0.0.1 (0 = none) */
unsigned serial_free_port(void);

/* starts the thread (it keeps trying to connect until stopped); NULL on
   error */
struct responder *serial_start(unsigned port);

/* stops the thread and writes DIR/NAME.serial (raw) and
   DIR/NAME.serial.txt (transcript) of every session; returns the number of
   sessions (-1 = a file could not be written) */
int serial_stop(struct responder *r, const char *dir);

#endif
