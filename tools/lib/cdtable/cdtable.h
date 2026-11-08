/*
 * cdtable.h - the CD result table (tests/data/blitz2-cd.tsv) written from
 * a fresh run, keeping the note rows.
 *
 * The table is TSV with the header
 *   path result line message size sha256_masked xtra note
 * and one row per source of the CD.  A note row is a row whose note
 * starts with "#NNN:" (an issue number): the executable of the port
 * differs from the CD's on purpose because of that fix.  The row holds the
 * port's values; the note names the CD's masked hash as "(2.10: HEX)"
 * (64 lowercase hex digits).
 *
 *   CDTABLE_UPDATE     FRESH is the port's run.  Every row is FRESH's.  A
 *                      row that was a note row keeps its note unless the
 *                      port's hash is the CD's again (a dropped note: the
 *                      fix no longer changes the executable).  A row that
 *                      is not a note row but differs from the old table is
 *                      reported (it needs a note by hand: the new value is
 *                      written).
 *   CDTABLE_REFERENCE  FRESH is the original compiler's run.  Every row is
 *                      FRESH's, except the note rows of OLD, which stay as
 *                      they are; their note must name FRESH's hash (else
 *                      the note is stale: error).
 *
 * Both fail (nothing written) on a note row of OLD that is not in FRESH or
 * has no hash in its note, and on tables that are not well formed (the
 * header, 8 fields per row, a path at most once).
 */
#ifndef CDTABLE_H
#define CDTABLE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum cdtable_mode { CDTABLE_UPDATE, CDTABLE_REFERENCE };

/*
 * The new table in *out (malloc'd), the messages (one per line: kept and
 * dropped notes, changed rows without a note, errors) in *report (malloc'd,
 * may be empty).  Return 0 = ok, 1 = errors (*out is NULL; the report says
 * which).
 */
int cdtable_merge(enum cdtable_mode mode, const char *fresh, const char *old,
                  char **out, char **report);

/* the CD's hash named by a note ("#NNN: ... (2.10: HEX)"): 1 and the 64
 * digits in hash[65] for a note row, 0 for another note, -1 for a note row
 * without a hash */
int cdtable_note_hash(const char *note, char hash[65]);

#ifdef __cplusplus
}
#endif

#endif
