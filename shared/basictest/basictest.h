/*
 * basictest.h - the codec of the result file of the Blitz test library
 * (spec testing/basic-test-interface, "Lines of the result file",
 * "Entries of the result file", "One codec for writer and reader").
 *
 * The result file holds one entry per line, fields separated by TAB, each
 * line ended by LF:
 *
 *   begin        VERSION                  the format version (1)
 *   block        init | case NAME | final the block the next entries
 *                                         belong to (NAME a string value,
 *                                         a field of its own)
 *   check        K ok|fail EXP ACT        a check of two longs
 *   checkq       K ok|fail EXP ACT        a check of two quicks (their bits)
 *   checkflt     K ok|fail EXP ACT DELTA  a check of two floats within a
 *                                         delta (their FFP bits)
 *   checkstr     K ok|fail EXP$ ELEN ECRC ACT$ ALEN ACRC CUT
 *                                         a check of two strings, each
 *                                         followed by the length and the
 *                                         CRC32 of the whole string; CUT
 *                                         `-` (both complete) or `cut` (a
 *                                         value shortened to fit the line)
 *   require      K broken                 a failed precondition
 *   expect-error K                        an announced runtime error
 *   blockend                              the end of the block
 *   broken       REASON                   the library's state is broken
 *   incomplete                            the finit found a block open
 *   end                                   the finit
 *   run          K                        a program is started (!RunProgram)
 *   runend       K RC|-                   it returned: its return code (a
 *                                         long value), `-` where the system
 *                                         gives none (Kickstart 1.x)
 *
 * K and a length are decimal (0 to 4294967295, no sign, no leading zero),
 * a long value (also the 32 bits of a quick or an FFP float) and a CRC32
 * `$` and 8 upper-case hex digits, a string value the bytes $20 to $7E
 * other than `\` as they are and every other byte as `\xHH` (upper-case
 * hex digits).  A string value is cut if it is shorter than its length
 * (it is the start of the string then); the reader refuses a value longer
 * than its length, a complete value whose CRC32 is not that of its bytes,
 * CUT `-` with a cut value and CUT `cut` without one.  The CRC32 is the
 * standard CRC-32 of IEEE 802.3 (basictest_crc32).
 *
 * Shared code like shared/tokenizer: every user compiles the files itself
 * with its own options - the host tools all (tools/lib/basictest), the
 * build the writer and the CRC32 (basictest_write.c, basictest_crc.c) with
 * the Blitz library conventions (cmake/BlitzLibrary.cmake), the form the
 * test library (libs/userlibs/testlib) links.  C99 on the freestanding
 * headers: no libc, no libgcc (no 32-bit multiply, divide or modulo on
 * the 68000), no writable static data, no allocation - the caller gives
 * the memory.  The build compiles writer and CRC32 in the library
 * environment and refuses them if they refer to any symbol they do not
 * define (shared/basictest/CMakeLists.txt).
 */
#ifndef BASICTEST_H
#define BASICTEST_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The format version the codec writes and reads (`begin`). */
#define BASICTEST_VERSION 1

/** @brief The entry kinds. */
enum basictest_kind {
    BASICTEST_BEGIN,
    BASICTEST_BLOCK,
    BASICTEST_CHECK,
    BASICTEST_CHECKSTR,
    BASICTEST_REQUIRE,
    BASICTEST_EXPECT_ERROR,
    BASICTEST_BLOCKEND,
    BASICTEST_BROKEN,
    BASICTEST_INCOMPLETE,
    BASICTEST_END,
    BASICTEST_CHECKQ,
    BASICTEST_CHECKFLT,
    BASICTEST_RUN,
    BASICTEST_RUNEND
};

/** @brief The blocks of a `block` entry. */
enum basictest_block {
    BASICTEST_BLOCK_INIT,       /* the batch init */
    BASICTEST_BLOCK_CASE,       /* a case, with its name */
    BASICTEST_BLOCK_FINAL       /* the batch cleanup */
};

/** @brief A string value: bytes, any value, not 0-terminated. */
struct basictest_str {
    const uint8_t *data;
    size_t length;
};

/**
 * @brief One entry. Only the members of its kind count:
 *   begin         version
 *   block         block, name (BASICTEST_BLOCK_CASE)
 *   check         k, ok, expected, actual
 *   checkq        k, ok, expected, actual (the quicks' bits)
 *   checkflt      k, ok, expected, actual, delta (the floats' FFP bits)
 *   checkstr      k, ok, expected_str, expected_length, expected_crc,
 *                 actual_str, actual_length, actual_crc, cut
 *   require       k (always broken)
 *   expect-error  k
 *   broken        reason
 *   run           k
 *   runend        k, ok (1: the return code is known), actual (it)
 */
struct basictest_entry {
    int kind;                   /* enum basictest_kind */
    uint32_t version;
    int block;                  /* enum basictest_block */
    struct basictest_str name;
    uint32_t k;
    int ok;                     /* 1 ok, 0 fail */
    uint32_t expected, actual;  /* the bits of the longs */
    uint32_t delta;             /* checkflt: the FFP bits of the delta */
    struct basictest_str expected_str, actual_str;
    uint32_t expected_length, expected_crc;   /* checkstr: of the whole */
    uint32_t actual_length, actual_crc;       /*  strings, cut or not */
    int cut;                    /* checkstr: 1 a value was shortened */
    struct basictest_str reason;
};

/**
 * @brief Writes the line of an entry, with its LF, to buf.
 * @return the number of bytes written, 0 if the line does not fit into
 *   size bytes or the entry is not one of the kinds above, or a case
 *   without a name (buf then holds no complete line).
 */
size_t basictest_write(const struct basictest_entry *e, uint8_t *buf, size_t size);

/**
 * @brief Writes v in decimal (no sign, no leading zero) to buf, which has
 *   room for 10 bytes; the writer's and the error text's helper.
 * @return the number of digits.
 */
size_t basictest_decimal(uint32_t v, uint8_t *buf);

/**
 * @brief The CRC32 of a string value: the standard CRC-32 of IEEE 802.3
 *   (zlib, PNG; the polynomial $EDB88320 reflected, $FFFFFFFF at the
 *   start and XORed at the end); $CBF43926 for "123456789", 0 for none.
 */
uint32_t basictest_crc32(const uint8_t *data, size_t length);

/** @brief Why the reader refused a line (BASICTEST_OK = 0). */
enum basictest_err {
    BASICTEST_OK,
    BASICTEST_E_KIND,           /* unknown entry */
    BASICTEST_E_FIELDS,         /* wrong number of fields for the entry */
    BASICTEST_E_VALUE,          /* malformed value */
    BASICTEST_E_VERSION         /* unknown format version */
};

/**
 * @brief The reader's error: line, reason, field (1 = the entry name) and
 *   the entry name as it stands in the line (valid while the line is).
 */
struct basictest_error {
    uint32_t line;
    int code;                   /* enum basictest_err */
    unsigned field;             /* the field at fault (E_VALUE, E_VERSION) */
    const uint8_t *kind;        /* the first field of the line */
    size_t kind_length;
};

/**
 * @brief Reads one line of the result file (without its LF) into e.
 *
 * The string values are decoded in place: e's strings point into line,
 * which the call changes. line_number is the number of the line in the
 * file, for the error.
 * @return BASICTEST_OK, or the reason; err (if not NULL) then names line,
 *   reason and field.
 */
int basictest_read(uint8_t *line, size_t length, uint32_t line_number,
                   struct basictest_entry *e, struct basictest_error *err);

/**
 * @brief Writes the text of a reader error ("line 3: check: malformed
 *   value in field 5"), 0-terminated, cut to fit size bytes (size > 0).
 *   The entry name is written as a string value.
 * @return the length of the text without the 0.
 */
size_t basictest_error_text(const struct basictest_error *err, char *buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif
