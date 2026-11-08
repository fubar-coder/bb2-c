/*
 * cdcache.h - the cache of a corpusrun cd pass (cache=PATH): a record of the
 * digest of the pass's inputs and the hashes of its outputs, written after
 * a successful run.  If the next run has the same inputs and all outputs
 * are still there unchanged, corpusrun cd does not start the emulator.  A test
 * cache, not a build check: anything missing or odd in the record means a
 * full run.
 *
 * The record (text):
 *   cdrun-cache 1
 *   inputs SHA256
 *   SHA256 PATH        one line per output, in the order of the pass
 */
#ifndef CDCACHE_H
#define CDCACHE_H

#include <stddef.h>

/* the digest of the inputs: the text (the pass file), each file of files
   (its path, size and contents) and each regular file below each
   directory of dirs (sorted relative paths, size and contents; symbolic
   links followed).  0, or -1 if one is missing or not readable. */
int cdcache_digest(const char *text, const char *const *files, size_t nfiles,
                   const char *const *dirs, size_t ndirs, char hex[65]);

/* 1 if the record at path says these outputs came from inputs with this
   digest and each output still has the hash of the record; else 0 (no
   record, another digest, a broken record, an output missing or
   changed). */
int cdcache_hit(const char *path, const char *digest, const char *const *outs, size_t nouts);

/* writes the record for the digest and the current outputs; 0, or -1 (an
   output missing, the record not written) */
int cdcache_write(const char *path, const char *digest, const char *const *outs, size_t nouts);

#endif
