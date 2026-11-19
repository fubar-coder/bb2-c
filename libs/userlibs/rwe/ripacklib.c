/*
 * ripacklib - userlib 73 (RWE, Reflective Images "PACK-library" v1.2):
 * ByteRun1 packing and ILBM/ICE unpacking in memory.
 *   PackPBM(source,dest,length)     UnPackPBM source,dest,length,destlength
 *   DeIce(source,dest)              (Pack-Ice 2.40 decruncher)
 *   UnpackIFF(source,bitmap#[,numlines,xy_offset])
 *   ILBMPalette(source,palette#[,offset])   ILBMGrab source,bitmap#,palette#
 *   LoadIFF(filename$,bitmap[,palette])      ChunkHeader(a$)
 *
 * C translation of rwelibsrc/packlib/Pack-Library8.bb (21/3/1995); the
 * declarations equal BUM10A/BLITZLIBS/RILIBS/RIPACKLIB.OBJ (ctest
 * rwe_declarations_ripacklib), the behaviour is compared with it in the
 * emulator (libs/tests/rwe/ripacklib.c).  Notes:
 * docs/rwelibs/port/ripacklib.md.
 *
 * Everything is C behind thunks; the original saves all registers but
 * d0 (MOVEM) in the packers and unpackers, so do the thunks (d1/a0/a1;
 * GCC keeps the rest).  The byte/word arithmetic of the original (DBRA on
 * words, byte counters, signed address comparisons, DIVU with overflow)
 * is reproduced, so corrupt or odd data give the same results.  LoadIFF
 * calls RIAmosFuncLib (NextBank, BLoad, Start, Erase), ILBMPalette memlib
 * (globalloc/globfree), through ALibJsr.  No assembler.
 *
 * Fixed (#772): PackPBM's failure (the packed data would not be smaller
 * than the source) returns -1; the original had no RTS there and fell
 * into UnPackPBM with d0 = -1, which unpacked from address -1 (then 0,
 * 1 ...) into dest, limited by the caller's d3 + dest.
 *
 * Fixed (#778): UnPackPBM tests both limits before each access
 * (unsigned) and stops a literal run at the end of the source; the
 * original tested after the access (signed): it read the byte at
 * source+length and wrote one byte past destlength.
 *
 * Fixed (#770): LoadIFF takes the first free bank of RIAmosFuncLib from
 * bank 0 on (NextBank(0), as RWE's V2.5 of the library); the original
 * asked for one from bank 51 on, which RIAmosFuncLib's default of 50
 * banks does not have (LoadIFF returned 0 unless BankLimit raised it).
 *
 * Fixed (#771): LoadIFF returns UnpackIFF's result (0 also when the
 * palette's ILBMPalette fails); the original returned -1 whenever the
 * file loaded.
 *
 * Fixed (#777): UnpackIFF refuses a BMHD with nPlanes 0 (BB_FALSE)
 * before it fills its table of plane pointers; the original stored 256
 * pointers into the table of 8, over its variables and the code behind.
 *
 * Fixed (#775): UnpackIFF takes the bytes of a row as ILBM stores them,
 * padded to words (((width + 15) >> 4) << 1), and tests that width
 * against the bitmap; the original took width/8 and read the BODY out of
 * step for every width that is no multiple of 16.
 *
 * Fixed (#774): UnpackIFF counts the bytes of a row in a word; the
 * original counted in a signed byte, so rows of 129 to 256 bytes were
 * skipped without reading and longer rows read out of step.
 *
 * Fixed (#776): UnpackIFF cuts a run at the end of the row (the source
 * still skips the whole run); the original wrote every run completely,
 * into the next row or behind the plane.
 *
 * Fixed (#773): UnpackIFF reads the compression byte of the BMHD: 0
 * copies the rows as they are, 1 unpacks ByteRun1, any other value is
 * refused (BB_FALSE); the original decoded every BODY as ByteRun1.
 *
 * Fixed (#769, #991, #1105): ILBMPalette with an offset makes a palette
 * that is too small (or has no colour block) grow to offset + CMAP
 * colours and keeps its colours; a CMAP below 3 bytes is refused and the
 * block is sized from the whole colour count.  The original replaced a
 * palette that was too small with one of the CMAP's size and ignored the
 * offset (#769), wrote the colours offset * 12 bytes into a new block of
 * the CMAP's size (#991), wrote 65536 colours for a CMAP below 3 bytes
 * and sized the block from the low word of the CMAP length (#1105).  The
 * new block is allocated before the old one is freed, so a failed
 * allocation leaves the palette as it was (the original kept the freed
 * block then, #768).
 *
 * Fixed (#766): UnpackIFF and ILBMPalette walk the chunks of a FORM at
 * the given address (UnpackIFF only a FORM ILBM, another FORM is
 * refused); the original's raw search (a long compared at every even
 * address: BMHD within 5120 bytes, BODY within 5120 bytes from the BMHD,
 * CMAP within 5122 bytes) is the fallback when the address holds no FORM
 * or the walk does not find the chunk.  The original found ids inside
 * other chunks and missed chunks behind a large one.
 *
 * BUG(orig), kept:
 *   - DeIce: the picture flag of Pack-Ice overwrites the bit buffer with
 *     the default count $0f9f, whose bits then give the "own count" flag
 *     (always set) and the first 6 bits of the count: the default is
 *     never used, and the count read is at least $3C00 (15361 groups of
 *     8 bytes converted below the end of the data) (#767).
 */
#include "blitzlib.h"
#include "blitzobj.h"
#include <graphics/gfx.h>
#include <datatypes/pictureclass.h>

/* RIAmosFuncLib (75): index n-1 of the token list of the CD's BSS2
   RIAMOSFUNCLIB.OBJ */
#define TOK_bload    ((LIB_riamosfunclib << 7) + 15)    /* form 1: d0 name, d1 bank */
#define TOK_start    ((LIB_riamosfunclib << 7) + 17)    /* d0 bank -> d0 address */
#define TOK_erase    ((LIB_riamosfunclib << 7) + 19)    /* d0 bank */
#define TOK_nextbank ((LIB_riamosfunclib << 7) + 22)    /* d0 first -> d0 bank or -1 */

/* entry points (thunks, defined below) */
void packpbm(void), deice(void), iff1(void), unpackiff(void);
void ilbmpalette(void), ilbmpalette2(void), ilbmgrab(void);
void ilbmloadnopal(void), ilbmload(void), chunkheader(void), unpackpbm(void);
static void libinit(void);
static void libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#ripacklib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_ripacklib, init, BL_NORET, finit, 0);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(packpbm, 0, 0);
BLITZ_NAME("PackPBM", "sourceaddr,destaddr,length");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(deice, 0, 0);
BLITZ_NAME("DeIce", "(sourceaddr,destaddr)");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_bitmaplib, BL_IA1|BL_PD1);
BLITZ_SUBS(iff1, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_bitmaplib, BL_IA1|BL_PD1);
BLITZ_SUBS(unpackiff, 0, 0);
BLITZ_NAME("UnpackIFF", "(sourceaddr,bitmap#[,numlines,xy_offset])");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_palettelib, BL_IA1|BL_PD1);
BLITZ_SUBS(ilbmpalette, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_palettelib, BL_IA1|BL_PD1);
BLITZ_SUBS(ilbmpalette2, 0, 0);
BLITZ_NAME("ILBMPalette", "(sourceaddr,palette#[,#offset)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_IA0|BL_PD1, LIB_palettelib, BL_IA1|BL_PD2);
BLITZ_SUBS(ilbmgrab, 0, 0);
BLITZ_NAME("ILBMGrab", "sourceaddr,bitmap#,palette#");

/* !subs {...,0,1}: error2 = 1 as in the original */
BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_IA0|BL_PD1);
BLITZ_SUBS(ilbmloadnopal, 0, 1);
BLITZ_ARGS(BL_STRING, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_IA0|BL_PD1, LIB_palettelib, BL_IA1|BL_PD2);
BLITZ_SUBS(ilbmload, 0, 1);
BLITZ_NAME("LoadIFF", "(filename$,bitmap[,palette])");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(chunkheader, 0, 0);
BLITZ_NAME("ChunkHeader", "(a$)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(unpackpbm, 0, 0);
BLITZ_NAME("UnPackPBM", "sourceaddr,destaddr,length,destlength");

BLITZ_NULLSUB_AT(finit, libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, libinit, 0, 0);

BLITZ_LIBFIN();

/* ---------------------------------------------------------------- data */

#define W(p) (*(UWORD *)(p))
#define LG(p) (*(ULONG *)(p))
#define ID(a, b, c, d) ((ULONG)(a) << 24 | (ULONG)(b) << 16 | (c) << 8 | (d))

/* a palette object of palettelib: struct bb_palette (blitzobj.h); ILBMPalette
   writes the colour block (count.w, 0.w, then the components as longs), kept
   as the long the original moves (PDATA), the number of colours and the low
   and high words of the first colour cycle as one long */
#define PDATA(p) (*(ULONG *)&(p)->pdata)

/**
 * @brief init: does nothing (RTS).
 */
static void BLITZ_ROUTINE libinit(void)
{
}

/**
 * @brief finit: does nothing (RTS).
 */
static void BLITZ_ROUTINE libfinit(void)
{
}

/**
 * @brief ChunkHeader(a$): the first long of a string, as an IFF id.
 * @param s  d0: the string (its first four bytes are read, whatever its
 *           length)
 * @return d0: the long
 */
static ULONG BLITZ_ROUTINE chunkheader_c(const UBYTE *s BL_REG(d0))
{
    return LG(s);
}
BLITZ_THUNK(chunkheader, chunkheader_c, "d1/a0-a1");

/* ------------------------------------------------------------ UnPackPBM */

/**
 * @brief UnPackPBM sourceaddr,destaddr,length,destlength: unpacks
 * ByteRun1 data.
 *
 * n >= 0 copies n+1 bytes, n < 0 (not -128) repeats the next byte 1-n
 * times, -128 is skipped; stops at the end of the source or of the
 * destination.  Fixed (#778): both limits are tested before each access,
 * unsigned, also inside a literal run: nothing at or behind
 * source+length is read, nothing at or behind dest+destlength written.
 * The original tested after the access and signed.
 * @param src   d0: the packed data
 * @param dst   d1: the destination
 * @param len   d2: the length of the packed data
 * @param dlen  d3: the size of the destination
 */
static void BLITZ_ROUTINE unpackpbm_c(const UBYTE *src BL_REG(d0), UBYTE *dst BL_REG(d1),
                                      ULONG len BL_REG(d2), ULONG dlen BL_REG(d3))
{
    ULONG left = len;                           /* source bytes left */
    UWORD n;

    while (left) {
        UBYTE b = *src++;
        left--;
        if (b == 0x80)
            continue;
        if (!(b & 0x80)) {
            n = (UWORD)(b + 1);
            do {
                if (!left || !dlen)
                    return;
                *dst++ = *src++;
                left--;
                dlen--;
            } while (--n);
        } else {
            UBYTE v;
            if (!left)
                return;
            v = *src++;
            left--;
            n = (UWORD)(257 - b);
            do {
                if (!dlen)
                    return;
                *dst++ = v;
                dlen--;
            } while (--n);
        }
    }
}
BLITZ_THUNK(unpackpbm, unpackpbm_c, "d0-d1/a0-a1");

/* -------------------------------------------------------------- PackPBM */

/**
 * @brief PackPBM(sourceaddr,destaddr,length): packs data with ByteRun1.
 *
 * Runs of at most 128 (127 in the counter).  Fixed (#772): the failure
 * returns -1; dest holds the packed bytes written up to it.  The
 * original fell into UnPackPBM with d0 = -1, which unpacked from address
 * -1 into dest, limited by the caller's d3.
 * @param src   d0: the source
 * @param dst   d1: the destination
 * @param size  d2: the length of the source
 * @return d0: the length of the packed data, or -1 if it would reach
 *         `length` bytes
 */
static ULONG BLITZ_ROUTINE packpbm_c(const UBYTE *src BL_REG(d0), UBYTE *dst BL_REG(d1),
                                     ULONG size BL_REG(d2))
{
    const UBYTE *in = src, *scan, *end = src + size;
    UBYTE *out = dst, *limit = dst + size;
    UBYTE byte, count, prev;
    UWORD n;

#define PUT(v) do { *out++ = (v); if (out == limit) goto failure; } while (0)
    for (;;) {
        if (in == end)                          /* check1 */
            return out - dst;
        scan = in;
        byte = *scan++;
        if (scan == end) {                      /* finish1: one byte */
            PUT(0);
            PUT(byte);
            return out - dst;
        }
        if (*scan != byte) {                    /* literals */
            int last = 0;
            count = 0;
            for (;;) {                          /* check3 */
                prev = byte;
                byte = *scan++;
                if (byte == prev)
                    break;
                count++;
                if (scan == end) {
                    last = 1;                   /* check41 */
                    break;
                }
                if (count == 127)
                    break;
            }
            PUT(count);
            n = count;
            do
                PUT(*in++);
            while (n-- != 0);
            if (last)
                return out - dst;
        } else {                                /* check5: a run */
            int last = 0;
            count = 0;
            in = scan;
            for (;;) {
                byte = *in++;
                if (byte != *scan) {
                    in--;                       /* check62 */
                    break;
                }
                count++;
                if (in == end) {
                    last = 1;                   /* check61 */
                    break;
                }
                if (count == 127)
                    break;
            }
            PUT((UBYTE)-count);
            PUT(*scan);
            if (last)
                return out - dst;
        }
    }
failure:
    return -1;                                  /* fixed #772: the RTS */
#undef PUT
}
BLITZ_THUNK(packpbm, packpbm_c, "d1/a0-a1");

/* ---------------------------------------------------------------- DeIce */

/* the state of the Pack-Ice decruncher: a5 (packed data, read backwards),
   a6 (output, written backwards), a4 (start of the output), d7.b (bits) */
struct ice {
    const UBYTE *src;
    UBYTE *dst, *start;
    UBYTE bits;
};

/* get_1_bit: ADD.b d7,d7 - when the byte is empty, the next one with the
   marker bit (X) shifted in */
static int ice_bit(struct ice *s)
{
    int c = s->bits >> 7;
    UBYTE b = (UBYTE)(s->bits << 1);

    if (b) {
        s->bits = b;
        return c;
    }
    b = *--s->src;
    s->bits = (UBYTE)(b << 1 | c);              /* ADDX.b d7,d7 */
    return b >> 7;
}

/* get_d0_bits: (UWORD)n + 1 bits into a word */
static UWORD ice_bits(struct ice *s, UWORD n)
{
    UWORD value = 0;

    do
        value = (UWORD)(value << 1 | ice_bit(s));
    while (n-- != 0);
    return value;
}

static const ULONG direkt_tab[10] = {
    0x7fff000e, 0x00ff0007, 0x00070002, 0x00030001, 0x00030001,
    270 - 1, 15 - 1, 8 - 1, 5 - 1, 2 - 1
};
static const BYTE length_tab[10] = { 9, 1, 0, -1, -1, 8, 4, 2, 1, 0 };
static const BYTE more_offset[4] = { 11, 4, 7, 0 };
static const WORD more_add[3] = { 0x11f, -1, 0x1f };

/* normal_bytes: literal runs and strings until the start is reached */
static void ice_normal(struct ice *s)
{
    for (;;) {
        if (ice_bit(s)) {
            ULONG value = 0;
            UWORD n;

            if (ice_bit(s)) {                   /* the length of the run */
                const ULONG *tab = direkt_tab + 5;
                WORD tries = 4;
                for (;;) {
                    ULONG entry = *--tab;
                    value = ice_bits(s, (UWORD)entry);
                    if ((UWORD)value != (UWORD)(entry >> 16))  /* DBNE */
                        break;
                    if (tries-- == 0)
                        break;
                }
                value += tab[5];                /* ADD.l 20(a1),d1 */
            }
            n = (UWORD)value;
            do                                  /* copy_direkt */
                *--s->dst = *--s->src;
            while (n-- != 0);
        }
        if (!((LONG)s->dst > (LONG)s->start))  /* test_if_end: BGT */
            return;
        {                                       /* strings */
            WORD lenidx = 3, entry, offidx;
            UWORD extra = 0, len, n;            /* d1: the extra length bits */
            UWORD offset;                       /* d1: the string offset */
            const UBYTE *from;

            while (ice_bit(s) && lenidx-- != 0) /* DBCC */
                ;
            entry = length_tab[1 + lenidx];
            if (entry >= 0)
                extra = ice_bits(s, (UWORD)entry);
            len = (UWORD)((UBYTE)length_tab[6 + lenidx] + extra);
            if (len) {
                offidx = 1;
                while (ice_bit(s) && offidx-- != 0)
                    ;
                offset = ice_bits(s, (UWORD)more_offset[1 + offidx]);
                offset = (UWORD)(offset + more_add[1 + offidx]);
                if ((WORD)offset < 0)           /* BPL / SUB.w d4,d1 */
                    offset = (UWORD)(offset - len);
            } else {                            /* get_offset_2 */
                WORD add = -1;
                UWORD bits = 5;
                if (ice_bit(s)) {
                    bits = 8;
                    add = 0x3f;
                }
                offset = (UWORD)(ice_bits(s, bits) + add);
            }
            from = s->dst + 2 + (WORD)len + (WORD)offset; /* depack_bytes */
            *--s->dst = *--from;
            n = len;
            do
                *--s->dst = *--from;
            while (n-- != 0);
        }
    }
}

/* getinfo: a long read byte by byte (the data may be at an odd address) */
static ULONG getinfo(const UBYTE *p)
{
    return (ULONG)p[0] << 24 | (ULONG)p[1] << 16 | (ULONG)p[2] << 8 | p[3];
}

/**
 * @brief DeIce(sourceaddr,destaddr): unpacks Pack-Ice 2.40 data.
 *
 * The data must start with "ICE!" (then the packed and the unpacked
 * length); the picture flag of Pack-Ice converts Atari ST low-res
 * bitplanes into interleaved words.  BUG(orig) (#767): the picture flag
 * overwrites the bit buffer with the default count $0F9F: the default is
 * never used, the count read is at least $3C00.
 * @param src  d0: the packed data
 * @param dst  d1: the destination
 * @return d0: BB_TRUE after unpacking, BB_FALSE without "ICE!"
 */
static BB_BOOL BLITZ_ROUTINE deice_c(const UBYTE *src BL_REG(d0), UBYTE *dst BL_REG(d1))
{
    struct ice s;
    UBYTE *end;
    UWORD count;
    UWORD *p;

    if (getinfo(src) != ID('I', 'C', 'E', '!'))
        return BB_FALSE;
    s.src = src + getinfo(src + 4);             /* LEA -8(a0,d0.l),a5 */
    end = dst + getinfo(src + 8);
    s.dst = end;
    s.start = dst;
    s.bits = *--s.src;                          /* MOVE.b -(a5),d7 */
    ice_normal(&s);
    /* the picture flag: Atari ST low-res bitplanes -> interleaved words */
    if (!ice_bit(&s))
        return BB_TRUE;
    /* BUG(orig): MOVE.w #$0f9f,d7 replaces the bit buffer (#767) */
    s.bits = 0x9f;
    count = 0x0f9f;
    if (ice_bit(&s))
        count = ice_bits(&s, 15);
    p = (UWORD *)end;
    do {
        UWORD c[4] = { 0, 0, 0, 0 };
        int i, j, k;
        for (i = 0; i < 4; i++) {
            UWORD w = *--p;
            for (j = 0; j < 4; j++)
                for (k = 0; k < 4; k++) {
                    c[k] = (UWORD)(c[k] << 1 | w >> 15);
                    w <<= 1;
                }
        }
        p[0] = c[0];
        p[1] = c[1];
        p[2] = c[2];
        p[3] = c[3];
    } while (count-- != 0);
    return BB_TRUE;
}
BLITZ_THUNK(deice, deice_c, "d1/a0-a1");

/* ------------------------------------------------------------ UnpackIFF */

/**
 * @brief The chunk `id` of a FORM, found by walking its chunks (fixed
 * #766).
 *
 * From the first chunk behind the FORM type up to the end of the FORM
 * (its length); a chunk of odd length is followed by its pad byte when
 * that byte is 0, otherwise the next chunk starts right behind the data
 * (a file written without pad, as #189) - an odd address, where the walk
 * stops (the long reads of the callers need even addresses; the raw
 * search, the fallback, does not look there either).
 * @param form  the FORM (its id was checked)
 * @param id    the chunk id
 * @return the chunk (its id), NULL if the FORM has none, a length runs
 *         past the FORM's end, or the walk reaches an odd address
 */
static const UBYTE *walkform(const UBYTE *form, ULONG id)
{
    const UBYTE *p = form + 12, *end = form + 8 + getinfo(form + 4);

    if (end < p)                                /* a length that wraps */
        return NULL;
    while ((ULONG)(end - p) >= 8) {
        ULONG len;
        if ((ULONG)p & 1)
            return NULL;
        if (LG(p) == id)
            return p;
        len = LG(p + 4);
        if (len > (ULONG)(end - p) - 8)
            return NULL;
        p += 8 + len;
        if ((len & 1) && p < end && *p == 0)
            p++;                                /* the pad byte */
    }
    return NULL;
}

/* the variables of the unpacker in the original's order */
static struct {
    ULONG iffsource, iffdest;
    UBYTE *bitplanes[8];
    UWORD iffheight, bytewidth, bitmapwidth;
    UBYTE iffdepth;
} BL_PACKED v;

/**
 * @brief UnpackIFF(sourceaddr,bitmap#,numlines,xy_offset): the BODY of an
 * ILBM in memory into a bitmap.
 *
 * Finds the BMHD and the BODY by walking the FORM ILBM at source (fixed
 * #766; another FORM fails), else by the raw search of the original (BMHD
 * 5120 bytes from source, BODY from the BMHD on, by 2 bytes), then unpacks numlines lines into
 * the planes, each plane pointer + xy_offset.  Fails without BMHD or BODY,
 * if the bitmap has fewer planes or bytes per row than the picture, or no
 * planes, or if the BMHD has no planes (fixed #777: the original stored
 * 256 plane pointers into its table of 8).  A row has the word-aligned
 * width of ILBM (fixed #775: the original's width/8), its bytes counted
 * in a word (fixed #774: the original's signed byte skipped rows of 129
 * bytes and more); a run is cut at the end of the row (fixed #776: the
 * original wrote it into the next row).  The BMHD's compression decides:
 * 0 rows as they are, 1 ByteRun1, anything else fails (fixed #773: the
 * original took every BODY as ByteRun1).
 * @param src    d0: the ILBM data (FORM)
 * @param bm     a1: the bitmap object (parameter 1)
 * @param lines  d2: the number of lines (word; < 0: the BMHD's height;
 *               at most the bitmap's)
 * @param xy     d3: the byte offset added to every plane
 * @return d0: BB_TRUE, BB_FALSE on failure
 */
static BB_BOOL BLITZ_ROUTINE unpackiff_c(const UBYTE *src BL_REG(d0),
                                         struct BitMap *bm BL_REG(a1),
                                         ULONG lines BL_REG(d2), ULONG xy BL_REG(d3))
{
    const UBYTE *chunk = NULL, *body = NULL;
    const struct BitMapHeader *hd;
    ULONG left = 5120;
    UWORD rowbytes;                             /* d7: the BMHD byte width */
    UWORD bodyleft;                             /* d7: the BODY search countdown */
    UWORD line;
    UBYTE planes, code, packed;

    /* fixed #766: a FORM ILBM is walked, another FORM refused; the raw
       search of the original is the fallback */
    if (getinfo(src) == ID('F', 'O', 'R', 'M')) {
        if (getinfo(src + 8) != ID('I', 'L', 'B', 'M'))
            return BB_FALSE;
        chunk = walkform(src, ID('B', 'M', 'H', 'D'));
        if (chunk)
            body = walkform(src, ID('B', 'O', 'D', 'Y'));
    }
    if (!chunk) {
        chunk = src;
        while (LG(chunk) != ID('B', 'M', 'H', 'D')) {
            chunk += 2;
            if ((left -= 2) == 0)
                return BB_FALSE;
        }
    }
    hd = (const struct BitMapHeader *)(chunk + 8);
    if ((WORD)lines < 0)
        BL_SETW(lines, hd->bmh_Height);        /* the height of the BMHD */
    if ((WORD)((UWORD)lines - bm->Rows) >= 0)  /* CMP.w 2(a1),d2:BMI */
        BL_SETW(lines, bm->Rows);
    BL_SETW(lines, lines - 1);
    if ((WORD)lines < 0)
        return BB_FALSE;
    v.iffheight = (UWORD)lines;
    planes = hd->bmh_Depth;
    if (!planes || planes > bm->Depth)          /* fixed #777: 0 refused */
        return BB_FALSE;
    /* fixed #773: 0 the rows as they are, 1 ByteRun1, nothing else */
    packed = hd->bmh_Compression;
    if (packed > cmpByteRun1)
        return BB_FALSE;
    v.iffdepth = (UBYTE)(planes - 1);
    v.bitmapwidth = bm->BytesPerRow;
    if (!LG(bm->Planes))                        /* MOVE.l 8(a1),d0 */
        return BB_FALSE;
    {
        const ULONG *pl = (const ULONG *)bm->Planes;
        UBYTE **to = v.bitplanes;
        UWORD n = v.iffdepth;
        do
            *to++ = (UBYTE *)(*pl++ + xy);
        while (n-- != 0);
    }
    /* fixed #775: the word-aligned ILBM row (the original: ASR.w #3) */
    rowbytes = (UWORD)((((ULONG)hd->bmh_Width + 15) >> 4) << 1);
    if (rowbytes > v.bitmapwidth)               /* BHI */
        return BB_FALSE;
    v.bytewidth = (UWORD)(rowbytes - 1);
    if (!body) {                                /* the fallback */
        body = (const UBYTE *)hd;
        bodyleft = 5120;
        while (LG(body) != ID('B', 'O', 'D', 'Y')) {
            body += 2;
            bodyleft -= 2;                      /* SUBQ.w #2:BMI */
            if ((WORD)bodyleft < 0)
                return BB_FALSE;
        }
    }
    body += 8;
    line = v.iffheight;
    do {
        UBYTE **pp = v.bitplanes;
        UWORD plane = v.iffdepth;
        do {
            UBYTE *out = *pp;
            /* the bytes of the row left: a word (fixed #774, was d5.b) */
            WORD left = (WORD)(v.bytewidth + 1);
            *pp++ += v.bitmapwidth;
            if (!packed) {                      /* fixed #773: uncompressed */
                do
                    *out++ = *body++;
                while (--left);
            }
            while (left > 0) {
                UWORD n, put;
                code = *body++;
                if (code == 0x80)
                    continue;
                /* fixed #776: a run is cut at the end of the row */
                if (!(code & 0x80)) {
                    n = (UWORD)(code + 1);
                    put = (WORD)n < left ? n : (UWORD)left;
                    left = (WORD)(left - n);
                    n -= put;                   /* the rest is skipped */
                    do
                        *out++ = *body++;
                    while (--put);
                    body += n;
                } else {
                    UBYTE b = *body++;
                    n = (UWORD)(257 - code);
                    put = (WORD)n < left ? n : (UWORD)left;
                    left = (WORD)(left - n);
                    do
                        *out++ = b;
                    while (--put);
                }
            }
        } while (plane-- != 0);
    } while (line-- != 0);
    return BB_TRUE;
}
BLITZ_THUNK(unpackiff, unpackiff_c, "d1/a0-a1");

/**
 * @brief UnpackIFF(sourceaddr,bitmap#): all lines, no offset
 * (MOVE.w #-1,d2:MOVEQ #0,d3).
 * @param src    d0: the ILBM data
 * @param bm     a1: the bitmap object
 * @param lines  d2: its low word is set to -1 (all lines)
 * @return d0: as UnpackIFF's long form
 */
static BB_BOOL BLITZ_ROUTINE iff1_c(const UBYTE *src BL_REG(d0), struct BitMap *bm BL_REG(a1),
                                    ULONG lines BL_REG(d2))
{
    BL_SETW(lines, 0xffff);
    return unpackiff_c(src, bm, lines, 0);
}
BLITZ_THUNK(iff1, iff1_c, "d1/a0-a1");

/* ---------------------------------------------------------- ILBMPalette */

/**
 * @brief ILBMPalette(sourceaddr,palette#,offset): the CMAP of an ILBM into
 * a palette object.
 *
 * Finds the CMAP by walking the FORM at source (fixed #766), else by the
 * raw search of the original (5122 bytes from source, by 2 bytes: also a
 * CMAP chunk at source itself), and writes its colours from colour `offset` on; offset < 0
 * gives the object a new colour block of the CMAP's size (the old one
 * freed: "this information is deleted").  Fixed (#769, #991): with an
 * offset, a palette with fewer than offset + CMAP colours (or none)
 * grows: a new block for offset + CMAP colours, the old colours copied
 * into it, the ones between cleared, the cycles kept (as palettelib's
 * DecodePalette since #226); the original replaced it with a block of
 * the CMAP's size and ignored the offset (#769), or for a palette
 * without a block wrote the colours offset * 12 bytes into a block of
 * the CMAP's size (#991).  Fixed (#1105): a CMAP of fewer than 3 bytes
 * is refused, the block is sized from the whole colour count; the
 * original wrote 65536 colours for a CMAP below 3 bytes and sized the
 * block from the low word of the CMAP length.  The new block is
 * allocated before the old one is freed.
 * @param src     d0: the ILBM data
 * @param obj     a1: the palette object (parameter 1)
 * @param offset  d2: the first colour, < 0: a new block
 * @return d0: BB_TRUE, BB_FALSE without CMAP, with a CMAP of no colour or
 *         of more than the 65535 colours of a palette, or without memory
 *         (the palette unchanged)
 */
static BB_BOOL BLITZ_ROUTINE ilbmpalette2_c(const UBYTE *src BL_REG(d0),
                                            struct bb_palette *obj BL_REG(a1),
                                            LONG offset BL_REG(d2))
{
    const UBYTE *cmap = NULL;
    LONG left = 5120;
    UBYTE *to, *old = (UBYTE *)PDATA(obj);
    ULONG cols, oldcols = obj->numcols, count, first;
    struct bl_regs r;

    /* fixed #766: a FORM (of any type) is walked; the raw search of the
       original is the fallback (it also takes a CMAP chunk itself) */
    if (getinfo(src) == ID('F', 'O', 'R', 'M'))
        cmap = walkform(src, ID('C', 'M', 'A', 'P'));
    if (!cmap) {
        cmap = src;
        while (LG(cmap) != ID('C', 'M', 'A', 'P')) {
            cmap += 2;
            if ((left -= 2) < 0)                /* SUBQ.l #2:BPL */
                return BB_FALSE;
        }
    }
    /* fixed #1105: the whole count; none, or more than a palette holds
       (the quotient would not fit DIVU's word), is refused */
    if (LG(cmap + 4) < 3 || LG(cmap + 4) >= 3UL * 0x10000UL)
        return BB_FALSE;
    cols = bl_divu_w(LG(cmap + 4), 3) & 0xffff;
    first = offset < 0 ? 0 : (ULONG)offset;
    if (first > 0xffffUL || cols > 0xffffUL - first)
        return BB_FALSE;                        /* more than a palette holds */
    count = first + cols;
    if (offset >= 0 && old && oldcols >= count) {
        to = old + 4;                           /* it fits: in place */
    } else {
        UBYTE *block;
        r = (struct bl_regs){ .d0 = bl_mulu_w((UWORD)count, 12) + 16,
                              .d1 = MEMF_PUBLIC | MEMF_CLEAR };
        block = (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
        if (!block)
            return BB_FALSE;
        if (offset >= 0 && old) {               /* fixed #769: it grows */
            const ULONG *from = (const ULONG *)(old + 4);
            ULONG *into = (ULONG *)(block + 4), k;
            for (k = bl_mulu_w((UWORD)oldcols, 3); k; k--)
                *into++ = *from++;
        } else
            *(ULONG *)&obj->cyc[0] = 0;         /* a new palette */
        if (old) {
            r = (struct bl_regs){ .d0 = bl_mulu_w((UWORD)oldcols, 12) + 16, .a1 = (ULONG)old };
            BLITZ_ALIBJSR(TOK_globfree, r);
        }
        PDATA(obj) = (ULONG)block;
        obj->numcols = (UWORD)count;
        LG(block) = 0;
        W(block) = (UWORD)count;
        to = block + 4;
    }
    to += bl_mulu_w((UWORD)first, 12);          /* fixed #991: inside the block */
    cmap += 8;
    do {
        int i;
        for (i = 0; i < 3; i++) {
            LG(to) = 0;
            *to = *cmap++;
            to += 4;
        }
    } while (--cols);
    return BB_TRUE;
}
BLITZ_THUNK(ilbmpalette2, ilbmpalette2_c, "d1/a0-a1");

/**
 * @brief ILBMPalette(sourceaddr,palette#): as with offset -1 (a new
 * colour block).
 * @param src  d0: the ILBM data
 * @param obj  a1: the palette object
 * @return d0: BB_TRUE, BB_FALSE without CMAP or memory
 */
static BB_BOOL BLITZ_ROUTINE ilbmpalette_c(const UBYTE *src BL_REG(d0),
                                           struct bb_palette *obj BL_REG(a1))
{
    return ilbmpalette2_c(src, obj, -1);
}
BLITZ_THUNK(ilbmpalette, ilbmpalette_c, "d1/a0-a1");

/**
 * @brief ILBMGrab sourceaddr,bitmap#,palette#: UnpackIFF(source,bitmap#)
 * and ILBMPalette(source,palette#); the results are dropped.
 * @param src    d0: the ILBM data
 * @param bm     a0: the bitmap object (parameter 1)
 * @param pal    a1: the palette object (parameter 2)
 * @param lines  d2: (the palette number) - UnpackIFF sets its low word
 *               to -1
 */
static void BLITZ_ROUTINE ilbmgrab_c(const UBYTE *src BL_REG(d0), struct BitMap *bm BL_REG(a0),
                                     struct bb_palette *pal BL_REG(a1), ULONG lines BL_REG(d2))
{
    iff1_c(src, bm, lines);
    ilbmpalette_c(src, pal);
}
BLITZ_THUNK(ilbmgrab, ilbmgrab_c, "d0-d1/a0-a1");

/* -------------------------------------------------------------- LoadIFF */

/**
 * @brief LoadIFF(filename$,bitmap,palette): loads an ILBM file into a
 * bitmap and a palette.
 *
 * The file goes into a free bank of RIAmosFuncLib (NextBank from 0 on,
 * fixed #770: the original's from 51 on needed BankLimit above 51; BLoad),
 * UnpackIFF and (with a palette) ILBMPalette from there, then the bank is
 * erased.  Fixed (#771): the result is UnpackIFF's, BB_FALSE also when
 * ILBMPalette fails; the original returned BB_TRUE whenever the file
 * loaded.
 * @param name  d0: the file name
 * @param bm    a0: the bitmap object (parameter 1)
 * @param pal   a1: the palette object (parameter 2), NULL: none
 * @return d0: BB_TRUE, BB_FALSE if there is no bank, the file does not
 *         load, or UnpackIFF or ILBMPalette fails
 */
static BB_BOOL BLITZ_ROUTINE ilbmload_c(ULONG name BL_REG(d0), struct BitMap *bm BL_REG(a0),
                                        struct bb_palette *pal BL_REG(a1))
{
    struct bl_regs r = { .d0 = 0 };             /* fixed #770: was 51 */
    ULONG bank, start;
    BB_BOOL ok;

    bank = BLITZ_ALIBJSR_FORM(TOK_nextbank, 1, r);
    if ((LONG)bank < 0)
        return BB_FALSE;
    if (!BL_JSR_D4_(BL_TOKVAL(TOK_bload, 1), name, bank, 0, 0)) {
        r = (struct bl_regs){ .d0 = bank };
        BLITZ_ALIBJSR(TOK_erase, r);
        return BB_FALSE;
    }
    r = (struct bl_regs){ .d0 = bank };
    start = BLITZ_ALIBJSR(TOK_start, r);
    ok = iff1_c((const UBYTE *)start, bm, 0);
    if (pal) {
        r = (struct bl_regs){ .d0 = bank };
        start = BLITZ_ALIBJSR(TOK_start, r);
        if (!ilbmpalette_c((const UBYTE *)start, pal))
            ok = BB_FALSE;
    }
    r = (struct bl_regs){ .d0 = bank };
    BLITZ_ALIBJSR(TOK_erase, r);
    return ok;                                  /* fixed #771: was BB_TRUE */
}
BLITZ_THUNK(ilbmload, ilbmload_c, "d1/a0-a1");

/**
 * @brief LoadIFF(filename$,bitmap): as above without a palette.
 * @param name  d0: the file name
 * @param bm    a0: the bitmap object
 * @return d0: as LoadIFF with a palette
 */
static BB_BOOL BLITZ_ROUTINE ilbmloadnopal_c(ULONG name BL_REG(d0), struct BitMap *bm BL_REG(a0))
{
    return ilbmload_c(name, bm, NULL);
}
BLITZ_THUNK(ilbmloadnopal, ilbmloadnopal_c, "d1/a0-a1");
