/*
 * xtra.h - the companion settings of a Blitz source (".xtra") as one
 * structure, with readers and writers for the formats that carry them
 * (docs/formats.md 1):
 *
 *   Blitz 2 .xtra        binary, vers 0 and 1 (blitz2.s:15290 optspnts,
 *                        loadxtra/savextra): read and written byte-exact
 *   BB22 source header   the editor part (cursor, top line) of the "BB22"
 *                        sources (docs/rwelibs/README.md 4.1): read
 *
 * Shared code like shared/filereq: every user compiles xtra.c itself
 * with its own options - the host tools (tools/lib/xtra), later blitz2
 * and TED (-fbaserel).  Pure C99
 * on the freestanding headers: no libc, no libgcc (no 32-bit multiply or
 * divide on the 68000), no writable static data, no allocation; the
 * caller provides the memory (a struct xtra of about 11 KB and the file
 * buffers).  The file formats are big-endian; the code assembles the
 * bytes itself and runs on big- and little-endian machines.
 *
 * Round trip: xtra_read_blitz2 keeps every byte of a valid file - the
 * fixed-size text fields as raw arrays (bytes after the 0 included), the
 * pointer field of each resident (a runtime value blitz2 clears on load,
 * 104 CD files have memory garbage in it) and the {lib, max} pairs in
 * file order - so xtra_write_blitz2 gives the same bytes again.
 *
 * Later (not here): AmiBlitz3's binary .xtra (15 switch words, the CLI
 * block's last 12 bytes as stack/version/builds, PED's XtraExtend after
 * the pairs) - its fields fit into struct xtra_ab3; a reader would set
 * source = XTRA_SRC_AB3_BINARY.
 */
#ifndef XTRA_H
#define XTRA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------ sizes */

#define XTRA_NSWITCH   14       /* switch words of the Blitz 2 file */
#define XTRA_NBUFFER   6        /* buffer sizes */
#define XTRA_NRES      8        /* residents (blitz2: mostres) */
#define XTRA_RESNAME   64       /* bytes of a resident's path */
#define XTRA_PATHLEN   192      /* bytes of the executable's path and name */
#define XTRA_CLILEN    128      /* bytes of the CLI argument (vers 1) */
#define XTRA_FIXED_V0  982      /* fixed part of a vers 0 file */
#define XTRA_FIXED_V1  1110     /* fixed part of a vers 1 file */
#ifndef XTRA_MAXMAXIMA
#define XTRA_MAXMAXIMA 256      /* object maxima kept; more is an error */
#endif
#define XTRA_MAXNAME   32       /* bytes of an object name (AmiBlitz3) */
#define XTRA_LABELLEN  32       /* bytes of AmiBlitz3's LabelSearch */

/* largest Blitz 2 file: the fixed part and XTRA_MAXMAXIMA pairs */
#define XTRA_BLITZ2_MAXSIZE (XTRA_FIXED_V1 + 4 * XTRA_MAXMAXIMA)

/* ------------------------------------------------------------ fields */

/* the switch words in file order (offset 2 + 2 * index); each is the
   Flags word of the option gadget: XTRA_ON (GFLG_SELECTED) = on */
enum xtra_switch {
    XTRA_SW_RUNTIME_ERRORS,     /* +2  optreqga10, AB3 RuntimeDebug */
    XTRA_SW_DEBUGGER,           /* +4  optreqga11 */
    XTRA_SW_LONG_ARRAYS,        /* +6  optreqga12 */
    XTRA_SW_EXPAND_MACROS,      /* +8  optreqga13 */
    XTRA_SW_QUOTED_MACROS,      /* +10 optreqga14 */
    XTRA_SW_MAKE_SMALLEST,      /* +12 optreqga15, AB3 MakeSmallest */
    XTRA_SW_DEBUG_INFO,         /* +14 optreqga16, AB3 DebugInfo */
    XTRA_SW_AUTO_RUN,           /* +16 optreq2ga1, AB3 AutoRun */
    XTRA_SW_INTERRUPT_CHECK,    /* +18 optreq2ga2, AB3 InterruptCheck */
    XTRA_SW_ASSEMBLER_CHECK,    /* +20 optreq2ga3, AB3 AssemblerCheck */
    XTRA_SW_OVERFLOW_CHECK,     /* +22 optreq2ga4, AB3 OverflowCheck */
    XTRA_SW_OPTREQ2GA5,         /* +24 optreq2ga5 (meaning open) */
    XTRA_SW_DEFAULT_INTERRUPT,  /* +26 optreq2ga6 */
    XTRA_SW_DEFAULT_OVERFLOW    /* +28 optreq2ga7 */
};
#define XTRA_ON 0x0080          /* GFLG_SELECTED; also the icon byte */

/* the buffer sizes in file order (offset 30 + 4 * index) */
enum xtra_buffer {
    XTRA_BUF_OBJECT, XTRA_BUF_LIBS, XTRA_BUF_SYSTEM, XTRA_BUF_DATA,
    XTRA_BUF_MACRO, XTRA_BUF_STRING
};

/* where the contents came from */
enum xtra_source {
    XTRA_SRC_NONE,              /* xtra_init only */
    XTRA_SRC_BLITZ2,            /* a Blitz 2 .xtra */
    XTRA_SRC_AB3_TEXT           /* an AmiBlitz3 embedded header */
    /* later: XTRA_SRC_AB3_BINARY */
};

struct xtra_resident {
    uint8_t name[XTRA_RESNAME]; /* 0-terminated path, raw (file bytes) */
    uint32_t ptr;               /* runtime value (resident's memory) as in
                                   the file; 0 when written from scratch */
};

/* one object maximum.  The Blitz 2 file names the library by number
   (flag XTRA_MAX_LIB), the AmiBlitz3 header the object by name
   (XTRA_MAX_NAME, e.g. "Screen"); mapping one to the other needs the
   libraries and is the caller's business (set lib/name and the flag). */
struct xtra_max {
    uint16_t lib;               /* library number (XTRA_MAX_LIB) */
    uint16_t max;               /* the maximum */
    uint8_t flags;              /* XTRA_MAX_LIB | XTRA_MAX_NAME */
    char name[XTRA_MAXNAME];    /* 0-terminated (XTRA_MAX_NAME) */
};
#define XTRA_MAX_LIB  0x01
#define XTRA_MAX_NAME 0x02

/* the editor part: not in a Blitz 2 .xtra; from a BB22 header or the
   AmiBlitz3 header.  All values 0-based. */
struct xtra_editor {
    uint8_t flags;              /* XTRA_ED_CURSOR, XTRA_ED_TOP */
    uint32_t line;              /* cursor line (XTRA_ED_CURSOR) */
    uint32_t column;            /* cursor column as shown (XTRA_ED_CURSOR) */
    uint32_t top;               /* first line shown (XTRA_ED_TOP) */
};
#define XTRA_ED_CURSOR 0x01
#define XTRA_ED_TOP    0x02

/* keys of the AmiBlitz3 header: bit (1 << key) in xtra_ab3.seen when the
   header had the key (docs/formats.md 7.2) */
enum xtra_ab3_key {
    XTRA_K_EXEPATH, XTRA_K_EXEFILE, XTRA_K_CREATEICON, XTRA_K_RESIDENTS,
    XTRA_K_STRINGBUFFER, XTRA_K_MAKESMALLEST, XTRA_K_FUNCOPTIMIZE,
    XTRA_K_VERSION, XTRA_K_NUMBEROFBUILDS, XTRA_K_CLIARGS, XTRA_K_STACKSIZE,
    XTRA_K_RUNTIMEDEBUG, XTRA_K_DEBUGINFO, XTRA_K_CREATEDBGFILE,
    XTRA_K_OVERFLOWCHECK, XTRA_K_ASSEMBLERCHECK, XTRA_K_INTERRUPTCHECK,
    XTRA_K_AUTORUN, XTRA_K_CURSORLINE, XTRA_K_CURSORCOLUMN,
    XTRA_K_LABELSEARCH, XTRA_K_LABELREMARK, XTRA_K_LABELALL,
    XTRA_K_LABELCASE, XTRA_K_LABELPOSITION, XTRA_K_MAX,
    XTRA_K_COUNT
};

/* fields only AmiBlitz3 has (present: the XTRA_K_* bit in seen; without
   it the value is AmiBlitz3's default from xtra_init) */
struct xtra_ab3 {
    uint32_t seen;              /* 1 << enum xtra_ab3_key */
    uint8_t header;             /* 1: the source had a header */
    uint32_t stack;             /* StackSize (default 32768) */
    uint8_t func_optimize;      /* FuncOptimize 0/1 */
    uint8_t create_dbgfile;     /* CreateDbgFile 0/1 */
    uint8_t version[3];         /* Version major.minor.patch */
    uint32_t builds;            /* NumberOfBuilds */
    char label_search[XTRA_LABELLEN];   /* LabelSearch, 0-terminated */
    uint16_t label_remark, label_all, label_case; /* words (comWord_*), as written */
    uint32_t label_position;    /* LabelPosition */
    /* lines "; KEY = VALUE" with a KEY the reader does not know: ignored,
       counted; the first one's line number (1-based) */
    uint32_t unknown;
    uint32_t unknown_line;
};

struct xtra {
    uint8_t source;             /* enum xtra_source */
    uint8_t vers;               /* 0 (no CLI block) or 1 */
    uint8_t icon;               /* executable with icon: XTRA_ON or 0 */
    uint16_t sw[XTRA_NSWITCH];  /* enum xtra_switch */
    uint32_t buf[XTRA_NBUFFER]; /* enum xtra_buffer, bytes */
    struct xtra_resident res[XTRA_NRES];
    uint8_t exe_path[XTRA_PATHLEN];     /* 0-terminated, raw */
    uint8_t exe_name[XTRA_PATHLEN];     /* 0-terminated, raw */
    uint8_t cli[XTRA_CLILEN];   /* 0-terminated, raw (vers 1) */
    uint16_t nmax;              /* entries in max[] */
    struct xtra_max max[XTRA_MAXMAXIMA];
    struct xtra_editor editor;
    struct xtra_ab3 ab3;
};

/* ------------------------------------------------------------ errors */

enum xtra_err {
    XTRA_OK,
    XTRA_E_SHORT,               /* shorter than the fixed part */
    XTRA_E_SIZE,                /* not fixed part + 4n */
    XTRA_E_VERS,                /* vers neither 0 nor 1 */
    XTRA_E_UNTERMINATED,        /* text field without its 0 byte */
    XTRA_E_TOO_MANY,            /* more than XTRA_MAXMAXIMA maxima */
    XTRA_E_SPACE,               /* output buffer too small */
    XTRA_E_BB22,                /* not a BB22 header we know */
    XTRA_E_AB3_LINE,            /* header line without ';' */
    XTRA_E_AB3_END,             /* header without "; /XTRA" */
    XTRA_E_AB3_NUMBER,          /* not a number / out of range */
    XTRA_E_AB3_QUOTE,           /* '"' without its closing '"' */
    XTRA_E_AB3_LONG,            /* text longer than its field */
    XTRA_E_AB3_RESIDENTS,       /* more than XTRA_NRES residents */
    XTRA_E_AB3_VERSION,         /* Version not N.N.N (0..255 each) */
    XTRA_E_AB3_NAME,            /* "Max" without object name */
    XTRA_E_COUNT
};

/* where an error is: byte offset (binary formats) or line (1-based, AB3) */
struct xtra_error {
    int code;                   /* enum xtra_err */
    size_t offset;
    uint32_t line;
};

/* the message of an error code (English, without position) */
const char *xtra_strerror(int code);

/* ------------------------------------------------------------ common */

/* Blitz 2's defaults (options.src, the initial gadget flags), no
   residents, empty paths and CLI argument, no maxima, no editor part,
   AmiBlitz3's defaults for its own fields; vers 1, source XTRA_SRC_NONE */
void xtra_init(struct xtra *x);

/* what blitz2's loadxtra does after reading a vers 0 file: Auto Run on,
   Interrupt/Overflow Checking from their defaults, Assembler Checking
   off (blitz2.s:15371).  The reader leaves the file values alone. */
void xtra_apply_vers0(struct xtra *x);

/* a maximum: by library number (lib >= 0) and/or by name (name != NULL);
   XTRA_E_TOO_MANY when full */
int xtra_add_max(struct xtra *x, int lib, const char *name, uint16_t max);

/* ------------------------------------------------------------ Blitz 2 */

/* reads a Blitz 2 .xtra (buf[0..size)) into x (initialised by the call;
   the editor part stays empty, source = XTRA_SRC_BLITZ2).  Strict: the
   size must be 982 + 4n (vers 0) or 1110 + 4n (vers 1), vers 0 or 1,
   every text field 0-terminated, at most XTRA_MAXMAXIMA pairs.  0 or
   enum xtra_err (err may be NULL). */
int xtra_read_blitz2(struct xtra *x, const uint8_t *buf, size_t size, struct xtra_error *err);

/* bytes xtra_write_blitz2 writes: the fixed part of x->vers and 4 per
   maximum with a library number */
size_t xtra_blitz2_size(const struct xtra *x);

/* writes x as Blitz 2 .xtra to out (cap bytes): vers as in x (0 without
   CLI block), the pairs of the maxima with XTRA_MAX_LIB in their order
   (the others are skipped: *skipped, may be NULL).  The size in *len.
   0 or XTRA_E_VERS / XTRA_E_SPACE. */
int xtra_write_blitz2(const struct xtra *x, uint8_t *out, size_t cap, size_t *len,
                      unsigned *skipped, struct xtra_error *err);

/* ------------------------------------------------------------ BB22 */

/* the editor part from the header of a BB22 source (14 or 16 bytes, the
   rules of tools/lib/bb22): line = top + row, column, top.  0 or
   XTRA_E_SHORT / XTRA_E_BB22. */
int xtra_read_bb22(struct xtra_editor *ed, const uint8_t *buf, size_t size, struct xtra_error *err);

#ifdef __cplusplus
}
#endif

#endif
