/*
 * cdtable.h - the CD result table (tests/data/blitz2-cd.tsv) written from
 * a fresh run, keeping the note rows.
 *
 * The table is TSV with the header
 *   path result line message size sha256_masked xtra note
 * and one row per source of the CD.  A note row is a row whose note
 * starts with "#NNN:" (an issue number): the port's result differs from
 * the CD's on purpose because of that fix.  The row holds the port's
 * values; the note names the CD's result: "(2.10: HEX)", the masked hash
 * (64 lowercase hex digits) of the CD's executable, or "(2.10: error
 * LINE)" where the CD's compiler refused the source at that line (the
 * fields result and line of an error row; LINE decimal without a leading
 * 0).  A row's result is the one a note names when it is an error row at
 * that line, or a row with that hash.
 *
 *   CDTABLE_UPDATE     FRESH is the port's run.  Every row is FRESH's.  A
 *                      row that was a note row keeps its note unless the
 *                      port's result is the CD's again (a dropped note: the
 *                      fix no longer changes the result).  A row that
 *                      is not a note row but differs from the old table is
 *                      reported (it needs a note by hand: the new value is
 *                      written).
 *   CDTABLE_REFERENCE  FRESH is the original compiler's run.  Every row is
 *                      FRESH's, except the note rows of OLD, which stay as
 *                      they are; their note must name FRESH's result
 *                      (else the note is stale: error).
 *
 * Both fail (nothing written) on a note row of OLD that is not in FRESH or
 * names no result in its note, and on tables that are not well formed (the
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

/* the longest result a note names: 64 hex digits or "error LINE", and 0 */
#define CDTABLE_REF_MAX 65

/* the CD's result named by a note ("#NNN: ... (2.10: HEX)" or "#NNN: ...
 * (2.10: error LINE)"): 1 and the 64 digits or "error LINE" in ref for a
 * note row, 0 for another note, -1 for a note row without the CD's result
 * (none of its "(2.10: " parts is well formed) */
int cdtable_note_ref(const char *note, char ref[CDTABLE_REF_MAX]);

#ifdef __cplusplus
}
#endif

#endif
