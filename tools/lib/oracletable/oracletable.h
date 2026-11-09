/*
 * oracletable.h - the oracle data of the TED tests
 * (tests/data/ted-oracle.sha256) written from a fresh run of the original
 * TED (target ted-oracle), keeping the note rows.
 *
 * One row per output of the original TED, sorted by KIND/PATH:
 *   HASH  KIND/PATH
 * (64 lowercase hex digits, two spaces, detok/, tokall/ or save/ and the
 * path on the CD).  A note row has a tab and a note behind it:
 *   HASH  KIND/PATH<TAB>#NNN: TEXT (TED 1.1: HASH)
 * the port's output differs from the original's on purpose because of
 * the fix of issue NNN: HASH is the port's, the note names the original
 * TED's hash.  The hash of a note row and the one its note names differ.
 *
 * The merge: every row is FRESH's (the original's run, no notes), except
 * the note rows of OLD, which stay as they are; their note must name
 * FRESH's hash (else the note is stale: error).  It fails (nothing
 * written) on a note row of OLD whose KIND/PATH is not in FRESH, a note
 * that is not of the form above, and on files that are not well formed (a
 * row not of the forms above, a KIND/PATH twice, a note in FRESH).
 */
#ifndef ORACLETABLE_H
#define ORACLETABLE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The new file in *out (malloc'd), the messages (one per line: kept notes,
 * errors) in *report (malloc'd, may be empty).  Return 0 = ok, 1 = errors
 * (*out is NULL; the report says which).
 */
int oracletable_merge(const char *fresh, const char *old, char **out, char **report);

/* the original's hash named by a note ("#NNN: ... (TED 1.1: HASH)"): 1
 * and the 64 digits and 0 in hash, -1 if the note is not of that form */
int oracletable_note_hash(const char *note, char hash[65]);

#ifdef __cplusplus
}
#endif

#endif
