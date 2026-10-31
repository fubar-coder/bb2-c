/*
 * xtra.h - the companion settings of a Blitz source (".xtra") as one small
 * structure of views and numbers, with a loader for the sources that carry
 * them and writers (docs/formats.md 1.4 and 7):
 *
 *   Blitz 2 .xtra        binary, vers 0 and 1 (blitz2.s:15290 optspnts,
 *                        loadxtra/savextra), the companion file NAME.xtra
 *                        of a TED, BB22 or text source: read, written
 *                        byte-exact
 *   BB22 source header   the editor part (cursor, top line) of the "BB22"
 *                        sources (docs/rwelibs/README.md 4.1): read
 *   AmiBlitz3 .ab3       the "Embedded .xtra Header" (comment lines at the
 *                        start of a text source, xtra_ab3.c): read; written
 *                        by updating the kept header (unchanged lines stay
 *                        as they are) or as a new header
 *
 * The caller does the I/O: it loads the source into memory, asks
 * xtra_detect for the type and xtra_companion_name for the name of the
 * companion .xtra (TED, BB22, plain text), reads that too (if it exists)
 * and passes both buffers to xtra_load.  The structure copies nothing: the
 * texts and the maxima are views - offset and length - into the .xtra
 * buffer or into the source header, the numbers are parsed.  xtra_load
 * gives the size of the source header (x->hdr); the caller keeps these
 * bytes (in front of the tokenised text or elsewhere: views are offsets,
 * so the bytes may move) and the .xtra buffer, and passes both as
 * struct xtra_base when reading a text, iterating the maxima or writing.
 *
 * Shared code like shared/filereq: every user compiles xtra.c and
 * xtra_ab3.c itself with its own options - the host tools (tools/lib/xtra),
 * later blitz2 and TED (-fbaserel).  Pure C99 on the freestanding
 * headers: no libc, no libgcc (no 32-bit multiply or divide on the 68000),
 * no writable static data, no allocation.  The file formats are
 * big-endian; the code assembles the bytes itself.
 *
 * AmiBlitz3's binary .xtra (15 switch words, the CLI block's last 12
 * bytes as stack/version/builds, PED's XtraExtend after the pairs) is
 * only recognised (xtra_is_ab3_xtra: the mode of a tokenised source in
 * blitz2-ab3); loading it is for later - its fields fit into struct
 * xtra_ab3.
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
#define XTRA_RESNAME   64       /* bytes of a resident's path field */
#define XTRA_PATHLEN   192      /* bytes of the executable's path and name */
#define XTRA_CLILEN    128      /* bytes of the CLI argument (vers 1) */
#define XTRA_LABELLEN  32       /* bytes of AmiBlitz3's LabelSearch */
#define XTRA_FIXED_V0  982      /* fixed part of a vers 0 file */
#define XTRA_FIXED_V1  1110     /* fixed part of a vers 1 file */
#define XTRA_AB3_NSWITCH 15     /* switch words of AmiBlitz3's binary .xtra */
#define XTRA_AB3_FIXED 1112     /* its fixed part (always with CLI block) */

/* ------------------------------------------------------------ fields */

/** @brief The switch words in file order (offset 2 + 2 * index); each is
    the Flags word of the option gadget: XTRA_ON (GFLG_SELECTED) = on. */
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

/** @brief The buffer sizes in file order (offset 30 + 4 * index). */
enum xtra_buffer {
    XTRA_BUF_OBJECT, XTRA_BUF_LIBS, XTRA_BUF_SYSTEM, XTRA_BUF_DATA,
    XTRA_BUF_MACRO, XTRA_BUF_STRING
};

/** @brief The type of a source file (xtra_detect, xtra_load). */
enum xtra_type {
    XTRA_T_TED,                 /* TED's tokenised source (has a $00);
                                   settings in NAME.xtra */
    XTRA_T_BB22,                /* RWE's BB22 source: 14/16-byte header
                                   (cursor); settings in NAME.xtra */
    XTRA_T_AB3,                 /* AmiBlitz3 text source: the settings in
                                   its embedded header (or AmiBlitz3's
                                   defaults without one); no NAME.xtra */
    XTRA_T_TEXT,                /* text without header; settings in
                                   NAME.xtra (Blitz 2 text sources) */
    XTRA_T_COUNT
};

/** @brief Where the settings came from. */
enum xtra_source {
    XTRA_SRC_NONE,              /* defaults only (xtra_init) */
    XTRA_SRC_BLITZ2,            /* a Blitz 2 .xtra */
    XTRA_SRC_AB3_TEXT           /* an AmiBlitz3 source (header or not) */
};

/** @brief Where the bytes of a view are; also the file of an error. */
enum xtra_in {
    XTRA_IN_NONE,               /* nothing: an empty text, no maxima */
    XTRA_IN_XTRA,               /* the .xtra buffer (xtra_base.xtra) */
    XTRA_IN_HEADER,             /* the source header (xtra_base.header);
                                   for an error: the source file */
    XTRA_IN_SET                 /* the caller's memory (xtra_set_text,
                                   xtra_set_maxima) or a constant */
};

/** @brief The bases of the views: the .xtra as loaded and the kept source
    header (src[0..x->hdr) of xtra_load, wherever it is now).

    Either may be NULL if no view points there. */
struct xtra_base {
    const uint8_t *xtra;
    const uint8_t *header;
};

/** @brief The text fields (xtra_text, xtra_set_text). */
enum xtra_field {
    XTRA_F_EXE_PATH,            /* .xtra 598, AB3 ExePath (< 192 bytes) */
    XTRA_F_EXE_NAME,            /* .xtra 790, AB3 ExeFile (< 192) */
    XTRA_F_CLI,                 /* .xtra 982 (vers 1), AB3 CliArgs (< 128) */
    XTRA_F_LABEL_SEARCH,        /* AB3 LabelSearch (< 32) */
    XTRA_F_RES0,                /* the residents: .xtra 54 + 68 * i, AB3
                                   Residents part i (< 64 each) */
    XTRA_F_COUNT = XTRA_F_RES0 + XTRA_NRES
};

/** @brief A text: a view (in, off, len) or the caller's bytes
    (XTRA_IN_SET: set, len).

    A view into the .xtra starts at its field: the Blitz 2 writer copies
    the whole field from there (bytes after the 0 included). */
struct xtra_text {
    const uint8_t *set;         /* XTRA_IN_SET: the bytes */
    uint32_t off;               /* XTRA_IN_XTRA/HEADER: offset in the base */
    uint16_t len;               /* bytes, without a 0 */
    uint8_t in;                 /* enum xtra_in */
};

/** @brief One object maximum.

    The Blitz 2 file names the library by number (XTRA_MAX_LIB), the
    AmiBlitz3 header the object by name (XTRA_MAX_NAME, as written there:
    a blank in the name is '-'); mapping one to the other needs the
    libraries and is the caller's business. */
struct xtra_max {
    const uint8_t *name;        /* XTRA_MAX_NAME: namelen bytes, no 0 */
    uint16_t namelen;
    uint16_t lib;               /* XTRA_MAX_LIB: library number */
    uint16_t max;               /* the maximum */
    uint8_t flags;              /* XTRA_MAX_LIB | XTRA_MAX_NAME */
};
#define XTRA_MAX_LIB  0x01
#define XTRA_MAX_NAME 0x02

/** @brief The maxima: the {lib, max} pairs of the .xtra (off: the first
    pair), the "Max NAME" lines of the header (off: the first one's line,
    n lines; a NAME given twice is there twice, the last one counts) or the
    caller's array (xtra_set_maxima).

    Read with xtra_max_begin/xtra_next_max. */
struct xtra_maxima {
    const struct xtra_max *set; /* XTRA_IN_SET */
    uint32_t off;
    uint32_t n;
    uint8_t in;                 /* enum xtra_in */
};

/** @brief The editor part: not in a Blitz 2 .xtra; from a BB22 header or
    the AmiBlitz3 header.

    All values 0-based. */
struct xtra_editor {
    uint8_t flags;              /* XTRA_ED_CURSOR, XTRA_ED_TOP */
    uint32_t line;              /* cursor line (XTRA_ED_CURSOR) */
    uint32_t column;            /* cursor column as shown (XTRA_ED_CURSOR) */
    uint32_t top;               /* first line shown (XTRA_ED_TOP) */
};
#define XTRA_ED_CURSOR 0x01
#define XTRA_ED_TOP    0x02

/** @brief Keys of the AmiBlitz3 header: bit (1 << key) in xtra_ab3.seen
    when the header has the key (docs/formats.md 7.2). */
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

/** @brief Numbers only AmiBlitz3 has (LabelSearch: XTRA_F_LABEL_SEARCH).

    Without the key in the header the value is AmiBlitz3's default. */
struct xtra_ab3 {
    uint32_t seen;              /* 1 << enum xtra_ab3_key: keys in the header */
    uint32_t stack;             /* StackSize (default 32768) */
    uint32_t builds;            /* NumberOfBuilds */
    uint32_t label_position;    /* LabelPosition */
    uint16_t label_remark, label_all, label_case; /* words, as written */
    uint8_t header;             /* 1: the source has a header */
    uint8_t func_optimize;      /* FuncOptimize 0/1 */
    uint8_t create_dbgfile;     /* CreateDbgFile 0/1 */
    uint8_t version[3];         /* Version major.minor.patch */
    /* lines "; KEY = VALUE" with a KEY the reader does not know: kept,
       counted; the first one's line number (1-based) */
    uint32_t unknown;
    uint32_t unknown_line;
};

/** @brief The settings.

    The numbers are members to read and set directly; the texts through
    xtra_text/xtra_set_text, the maxima through the iterator and
    xtra_set_maxima. */
struct xtra {
    uint8_t type;               /* enum xtra_type (xtra_load) */
    uint8_t source;             /* enum xtra_source */
    uint8_t vers;               /* 0 (no CLI block) or 1 */
    uint8_t icon;               /* executable with icon: XTRA_ON or 0 */
    uint32_t hdr;               /* bytes of the source header to keep:
                                   AB3 "; XTRA" to "; /XTRA" (with its line
                                   end), BB22 14 or 16; 0 for the others */
    uint16_t sw[XTRA_NSWITCH];  /* enum xtra_switch */
    uint32_t buf[XTRA_NBUFFER]; /* enum xtra_buffer, bytes */
    uint32_t res_ptr[XTRA_NRES]; /* the residents' pointer fields: runtime
                                   values as in the file (0 from scratch) */
    struct xtra_text text[XTRA_F_COUNT];
    struct xtra_maxima max;
    struct xtra_editor editor;
    struct xtra_ab3 ab3;
};

/* ------------------------------------------------------------ errors */

/** @brief The error codes of the functions (XTRA_OK = 0: no error). */
enum xtra_err {
    XTRA_OK,
    XTRA_E_SHORT,               /* shorter than the fixed part */
    XTRA_E_SIZE,                /* not fixed part + 4n */
    XTRA_E_VERS,                /* vers neither 0 nor 1 */
    XTRA_E_UNTERMINATED,        /* text field without its 0 byte */
    XTRA_E_SPACE,               /* output buffer too small */
    XTRA_E_BB22,                /* not a BB22 header we know */
    XTRA_E_AB3_LINE,            /* header line without ';' */
    XTRA_E_AB3_END,             /* header without "; /XTRA" */
    XTRA_E_AB3_NUMBER,          /* not a number / out of range */
    XTRA_E_AB3_QUOTE,           /* '"' without its closing '"' */
    XTRA_E_LONG,                /* text longer than its field */
    XTRA_E_AB3_RESIDENTS,       /* more than XTRA_NRES residents */
    XTRA_E_AB3_VERSION,         /* Version not N.N.N (0..255 each) */
    XTRA_E_AB3_NAME,            /* "Max" without object name */
    XTRA_E_TYPE,                /* not an enum xtra_type */
    XTRA_E_FIELD,               /* not an enum xtra_field */
    XTRA_E_TEXT,                /* a text with a 0, CR or LF byte */
    XTRA_E_BASE,                /* the bytes of a view not given (base NULL) */
    XTRA_E_COUNT
};

/** @brief Where an error is: file (XTRA_IN_XTRA or XTRA_IN_HEADER = the
    source), byte offset (binary formats) or line (1-based, AB3). */
struct xtra_error {
    int code;                   /* enum xtra_err */
    int in;                     /* enum xtra_in */
    size_t offset;
    uint32_t line;
};

/**
 * @brief The message of an error code (English, without position).
 * @param code  an enum xtra_err
 * @return the message; "unknown error" for another code
 */
const char *xtra_strerror(int code);

/* ------------------------------------------------------------ loading */

/**
 * @brief The type of a source.
 *
 * "BB22" at the start: XTRA_T_BB22; a $00 byte: XTRA_T_TED; first line
 * "; XTRA": XTRA_T_AB3; other text: XTRA_T_AB3 if path ends in ".ab3"
 * (any case, AmiBlitz3 reads every .ab3 so), else XTRA_T_TEXT.  The
 * caller may pass another type to xtra_load.
 *
 * @param buf   the source
 * @param len   its length
 * @param path  its file name (may be NULL)
 * @return an enum xtra_type
 */
int xtra_detect(const uint8_t *buf, size_t len, const char *path);

/**
 * @brief Whether the settings of a source of this type are in NAME.xtra.
 * @param type  an enum xtra_type
 * @return 1 for XTRA_T_TED, XTRA_T_BB22 and XTRA_T_TEXT, else 0
 */
int xtra_has_companion(int type);

/**
 * @brief The name of the companion of the source path: path + ".xtra" (as
 *        TED and blitz2 make it).
 * @param path  the source's file name
 * @param out   the name, 0-terminated
 * @param cap   the size of out in bytes
 * @return 0 or XTRA_E_SPACE
 */
int xtra_companion_name(const char *path, char *out, size_t cap);

/**
 * @brief Whether a companion .xtra has AmiBlitz3's binary format with 15
 *        switch words (since AmiBlitz/AmiBlitz3@bdae3f9e, docs/formats.md
 *        1.3) - not Blitz 2's 14.
 *
 * The layout as AmiBlitz3's write_xtrafile writes it: vers 1, icon, 15
 * switch words, 6 buffer sizes at 32, 8 residents at 56, path and name of
 * the executable at 600 and 792, CLI argument (116 bytes) and stack,
 * version, builds at 984 (fixed part XTRA_AB3_FIXED), the {lib, max}
 * pairs, 4 more bytes, maybe PED's XtraExtend.  Recognised by: at least
 * the fixed part + 4 bytes, a multiple of 4 (a Blitz 2 .xtra is 982/1110
 * + 4n), vers 1, icon and switch words 0 or XTRA_ON, every buffer size
 * below 16 MB (in the 14-word layout's place a buffer's low word would be
 * the next one's high word), the text fields 0-terminated.  An older
 * AmiBlitz3 .xtra with 14 switch words is not recognised.
 *
 * @param xtra  the file, xtra[0..len) (NULL: none)
 * @param len   its length
 * @return 1 or 0
 */
int xtra_is_ab3_xtra(const uint8_t *xtra, size_t len);

/** @brief The mode a program is compiled in by blitz2-ab3 (xtra_mode). */
enum xtra_mode {
    XTRA_MODE_DETECT,           /* force: no mode given, detect it */
    XTRA_MODE_BB2,              /* Blitz Basic 2: its tokens and libraries */
    XTRA_MODE_AB3               /* AmiBlitz3 v3.14.0's tokens and libraries */
};

/**
 * @brief The mode of a source (xtra_mode.c; docs/ab3/variant.md).
 *
 * force XTRA_MODE_BB2 or XTRA_MODE_AB3 (the command line's -mode): that
 * mode.  Otherwise by xtra_detect: XTRA_T_AB3 (a text with the embedded
 * header, or a text named *.ab3) is AB3; XTRA_T_TED (tokenised) is AB3 if
 * its companion .xtra has AmiBlitz3's format (xtra_is_ab3_xtra), else
 * BB2; everything else (BB22, text without header) is BB2.  The same rule
 * gives the mode of a tokenised include.
 *
 * @param force  an enum xtra_mode; XTRA_MODE_DETECT (or another value):
 *               detect
 * @param src    the source, src[0..len)
 * @param len    its length
 * @param path   its file name (may be NULL)
 * @param xtra   its companion .xtra, xtra[0..xlen) (NULL: none)
 * @param xlen   its length
 * @return XTRA_MODE_BB2 or XTRA_MODE_AB3
 */
int xtra_mode(int force, const uint8_t *src, size_t len, const char *path, const uint8_t *xtra,
              size_t xlen);

/**
 * @brief Load the settings of a source.
 *
 * x is initialised (xtra_init) and gets:
 *   - XTRA_T_TED, XTRA_T_TEXT: the .xtra (source XTRA_SRC_BLITZ2) or the
 *     defaults (XTRA_SRC_NONE); hdr 0
 *   - XTRA_T_BB22: the header's editor part, hdr 14/16; the .xtra
 *   - XTRA_T_AB3: the embedded header (XTRA_SRC_AB3_TEXT, its size in
 *     hdr) or AmiBlitz3's defaults (hdr 0)
 *
 * The views point into the .xtra and into src[0..x->hdr) - the caller
 * keeps both (struct xtra_base).  Strict: a .xtra must be 982 + 4n
 * (vers 0) or 1110 + 4n bytes (vers 1) with 0-terminated text fields; the
 * AB3 header must be well-formed (docs/formats.md 7.4).
 *
 * @param x     the settings
 * @param type  an enum xtra_type
 * @param src   the source, src[0..len) (may be NULL for XTRA_T_TED/
 *              XTRA_T_TEXT)
 * @param len   its length
 * @param xtra  its companion .xtra, xtra[0..xlen) (NULL: none; ignored for
 *              XTRA_T_AB3)
 * @param xlen  its length
 * @param err   the place of an error (may be NULL)
 * @return 0 or an enum xtra_err; on an error x holds the defaults
 */
int xtra_load(struct xtra *x, int type, const uint8_t *src, size_t len, const uint8_t *xtra,
              size_t xlen, struct xtra_error *err);

/* ------------------------------------------------------------ fields */

/**
 * @brief Initialise settings with the defaults.
 *
 * Blitz 2's defaults (options.src, the initial gadget flags), no
 * residents, empty texts, no maxima, no editor part, AmiBlitz3's defaults
 * for its own numbers; vers 1, type XTRA_T_TED, XTRA_SRC_NONE.
 *
 * @param x  the settings
 */
void xtra_init(struct xtra *x);

/**
 * @brief What blitz2's loadxtra does after reading a vers 0 file.
 *
 * Auto Run on, Interrupt/Overflow Checking from their defaults, Assembler
 * Checking off (blitz2.s:15371).  The loader leaves the file values alone.
 *
 * @param x  the settings
 */
void xtra_apply_vers0(struct xtra *x);

/**
 * @brief A text field.
 * @param x      the settings
 * @param b      the bases of the views (may be NULL)
 * @param field  an enum xtra_field
 * @param len    out: the length
 * @return its bytes (not 0-terminated); "" for an empty one; NULL (and
 *         *len 0) for a field number out of range or a view whose base b
 *         does not give
 */
const uint8_t *xtra_text(const struct xtra *x, const struct xtra_base *b, int field, size_t *len);

/**
 * @brief The bytes of a text field in a Blitz 2 .xtra (with the 0): a text
 *        is shorter than this.
 * @param field  an enum xtra_field
 * @return the size; 0 for a field number out of range
 */
size_t xtra_text_cap(int field);

/**
 * @brief Set a text field to the caller's bytes.
 * @param x      the settings
 * @param field  an enum xtra_field
 * @param p      the text, p[0..len) - kept by the caller as long as x is
 *               used (a writer reads it from there)
 * @param len    its length
 * @return 0, XTRA_E_FIELD, XTRA_E_LONG (len >= xtra_text_cap) or
 *         XTRA_E_TEXT (a 0, CR or LF)
 */
int xtra_set_text(struct xtra *x, int field, const void *p, size_t len);

/**
 * @brief The maxima from the caller's array instead of those of the
 *        .xtra/header.
 *
 * The AmiBlitz3 writer then replaces all "Max" lines (written in front of
 * "; /XTRA").
 *
 * @param x  the settings
 * @param m  the maxima, m[0..n) (kept by the caller)
 * @param n  their number; 0: none
 */
void xtra_set_maxima(struct xtra *x, const struct xtra_max *m, uint32_t n);

/**
 * @brief An iterator over the maxima, one by one, in file order.
 *
 *     struct xtra_iter it;
 *     struct xtra_max m;
 *     xtra_max_begin(&it, x, b);
 *     while (xtra_next_max(&it, &m)) ...
 *
 * No maxima from a view whose base b does not give.
 */
struct xtra_iter {
    const struct xtra *x;
    const uint8_t *base;        /* the .xtra or the header */
    const struct xtra_max *set; /* the next one of the caller's array */
    uint32_t pos, left;
};

/**
 * @brief Start iterating the maxima of x.
 * @param it  the iterator
 * @param x   the settings
 * @param b   the bases of the views
 */
void xtra_max_begin(struct xtra_iter *it, const struct xtra *x, const struct xtra_base *b);

/**
 * @brief The next maximum.
 * @param it  the iterator (xtra_max_begin)
 * @param m   out: the maximum
 * @return 1 with *m set, 0 at the end
 */
int xtra_next_max(struct xtra_iter *it, struct xtra_max *m);

/**
 * @brief Take what a Blitz 2 .xtra holds from from into to.
 *
 * vers, icon, switches, buffers, residents (texts, pointers), executable
 * path and name, CLI argument - not the maxima, the editor part,
 * AmiBlitz3's own fields.  from's views keep pointing into its .xtra: the
 * base used with to needs from's .xtra (e.g. an .ab3 header updated from
 * a .xtra).
 *
 * @param to    the settings to change
 * @param from  the settings to take from
 */
void xtra_take_blitz2(struct xtra *to, const struct xtra *from);

/* ------------------------------------------------------------ Blitz 2 */

/**
 * @brief The bytes xtra_write_blitz2 writes: the fixed part of x->vers and
 *        4 per maximum with a library number.
 * @param x  the settings
 * @return the size
 */
size_t xtra_blitz2_size(const struct xtra *x);

/**
 * @brief Write x as Blitz 2 .xtra.
 *
 * vers as in x (0 without CLI block); a text from the .xtra as its whole
 * field (the bytes after the 0 included: a loaded file gives the same
 * bytes), others 0-terminated and padded with 0; the pairs of the maxima
 * with XTRA_MAX_LIB in their order (the others skipped).
 *
 * @param x        the settings
 * @param b        the bases of the views
 * @param out      the output
 * @param cap      its size in bytes
 * @param len      out: the size written
 * @param skipped  out: the maxima skipped (may be NULL)
 * @param err      the place of an error (may be NULL)
 * @return 0 or XTRA_E_VERS, XTRA_E_SPACE, XTRA_E_LONG, XTRA_E_BASE
 */
int xtra_write_blitz2(const struct xtra *x, const struct xtra_base *b, uint8_t *out, size_t cap,
                      size_t *len, unsigned *skipped, struct xtra_error *err);

/* ------------------------------------------------------------ AmiBlitz3 */

/**
 * @brief Write x as AmiBlitz3 header (not 0-terminated).
 *
 *   - x from a source with header (x->ab3.header): that header
 *     (b->header, x->hdr bytes), line by line: unchanged lines as they
 *     are (unknown keys, comments, older layouts, the line ends); the line
 *     of a known key whose value in x differs from what the line says gets
 *     the new value (the rest of the line kept); keys the header lacks
 *     whose value differs from AmiBlitz3's default are added in front of
 *     "; /XTRA"; maxima set by xtra_set_maxima replace the "Max" lines.
 *     Unchanged x: the same bytes.
 *   - otherwise a new header, the lines of AmiBlitz3's write_AB3Header
 *     (docs/formats.md 7.1), LF line ends.
 *
 * @param x        the settings
 * @param b        the bases of the views
 * @param out      the output
 * @param cap      its size in bytes
 * @param len      out: the size (also when it does not fit)
 * @param skipped  out: the maxima without a name, skipped (may be NULL)
 * @return 0 or XTRA_E_SPACE, XTRA_E_BASE
 */
int xtra_write_ab3(const struct xtra *x, const struct xtra_base *b, char *out, size_t cap,
                   size_t *len, unsigned *skipped);

/**
 * @brief The bytes xtra_write_ab3 writes.
 * @param x  the settings
 * @param b  the bases of the views
 * @return the size; 0 on XTRA_E_BASE
 */
size_t xtra_ab3_size(const struct xtra *x, const struct xtra_base *b);

/**
 * @brief The name of an AmiBlitz3 key ("ExePath", ..., "Max").
 * @param key  an enum xtra_ab3_key
 * @return the name; "" for another number
 */
const char *xtra_ab3_keyname(int key);

#ifdef __cplusplus
}
#endif

#endif
