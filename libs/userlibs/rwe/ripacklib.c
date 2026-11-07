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
 * BUG(orig), kept:
 *   - PackPBM's failure (the packed data would not be smaller than the
 *     source: -1) falls into UnPackPBM with the restored registers and
 *     d0 = -1: it unpacks from address -1 (then 0, 1 ...) into dest,
 *     limited by the CALLER'S d3 + dest, before returning -1 (#772).
 *   - UnPackPBM reads the byte at source+length and writes one byte
 *     past destlength (the limits are tested after the access, signed)
 *     (#778).
 *   - UnpackIFF treats every BODY as ByteRun1 (the compression byte of
 *     the BMHD is not read) (#773), takes the row width as width/8 (rows
 *     that ILBM pads to words lose their alignment) (#775), counts the
 *     bytes of a row in a signed byte (rows of 129 bytes and more are
 *     skipped) (#774) and does not stop a run at the end of the row
 *     (#776); nPlanes 0 stores 256 plane pointers into its table of 8
 *     (#777).
 *   - The chunk searches step by 2 bytes from the given address (BMHD:
 *     5120 bytes, BODY: from the BMHD on, word counter; CMAP 5122 bytes)
 *     (#766).
 *   - ILBMPalette without offset always frees the palette's colour block
 *     (when there is one) and allocates a new one; with an offset into a
 *     palette that is too small it does the same and ignores the offset
 *     (#769).  The size of the freed block comes from the object's colour
 *     count; if the new allocation fails, the object keeps the freed
 *     block (#768).
 *   - LoadIFF returns -1 even if UnpackIFF failed (#771).
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

/* #ia<a>|#pd<p>: object `p` of the library into a<a> */
#define IA(a, p) (BL_LA(a) | BL_ITEM | BL_PD(p))

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
BLITZ_LIBS(LIB_bitmaplib, IA(1, 1));
BLITZ_SUBS(iff1, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_bitmaplib, IA(1, 1));
BLITZ_SUBS(unpackiff, 0, 0);
BLITZ_NAME("UnpackIFF", "(sourceaddr,bitmap#[,numlines,xy_offset])");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_palettelib, IA(1, 1));
BLITZ_SUBS(ilbmpalette, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_palettelib, IA(1, 1));
BLITZ_SUBS(ilbmpalette2, 0, 0);
BLITZ_NAME("ILBMPalette", "(sourceaddr,palette#[,#offset)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, IA(0, 1), LIB_palettelib, IA(1, 2));
BLITZ_SUBS(ilbmgrab, 0, 0);
BLITZ_NAME("ILBMGrab", "sourceaddr,bitmap#,palette#");

/* !subs {...,0,1}: error2 = 1 as in the original */
BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, IA(0, 1));
BLITZ_SUBS(ilbmloadnopal, 0, 1);
BLITZ_ARGS(BL_STRING, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, IA(0, 1), LIB_palettelib, IA(1, 2));
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
 * destination.  BUG(orig) (#778): the end tests come after the access
 * and compare signed - the byte at source+length is read, one byte past
 * destlength written.  Also the tail of PackPBM's failure (#772).
 * @param src   d0: the packed data
 * @param dst   d1: the destination
 * @param len   d2: the length of the packed data
 * @param dlen  d3: the size of the destination
 */
static void BLITZ_ROUTINE unpackpbm_c(const UBYTE *src BL_REG(d0), UBYTE *dst BL_REG(d1),
                                      ULONG len BL_REG(d2), ULONG dlen BL_REG(d3))
{
    /* addresses as numbers: PackPBM's failure starts at -1 */
    const UBYTE *end = (const UBYTE *)((ULONG)src + len);
    UBYTE *dend = (UBYTE *)((ULONG)dst + dlen);
    UWORD n;

    for (;;) {
        UBYTE b = *src++;
        if ((LONG)end < (LONG)src)              /* CMP.l a0,a6:BLT */
            return;
        if (b == 0x80)
            continue;
        if (!(b & 0x80)) {
            n = b;
            do {
                *dst++ = *src++;
                if ((LONG)dend < (LONG)dst)
                    return;
            } while (n-- != 0);
        } else {
            UBYTE v = *src++;
            n = (UBYTE)-b;                      /* NEG.b, upper bits 0 */
            do {
                *dst++ = v;
                if ((LONG)dend < (LONG)dst)
                    return;
            } while (n-- != 0);
        }
    }
}
BLITZ_THUNK(unpackpbm, unpackpbm_c, "d0-d1/a0-a1");

/* -------------------------------------------------------------- PackPBM */

/**
 * @brief PackPBM(sourceaddr,destaddr,length): packs data with ByteRun1.
 *
 * Runs of at most 128 (127 in the counter).  BUG(orig) (#772): the
 * failure falls into UnPackPBM with d0 = -1, which unpacks from address
 * -1 into dest, limited by the caller's d3, before -1 is returned.
 * @param src   d0: the source
 * @param dst   d1: the destination
 * @param size  d2: the length of the source
 * @param dlen  d3: only used by the failure (UnPackPBM's destlength)
 * @return d0: the length of the packed data, or -1 if it would reach
 *         `length` bytes
 */
static ULONG BLITZ_ROUTINE packpbm_c(const UBYTE *src BL_REG(d0), UBYTE *dst BL_REG(d1),
                                     ULONG size BL_REG(d2), ULONG dlen BL_REG(d3))
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
    /* BUG(orig): no RTS - into _UnPackPBM with d0 = -1 (#772) */
    unpackpbm_c((const UBYTE *)-1, dst, size, dlen);
    return -1;
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

/* the variables of the unpacker in the original's order (nPlanes 0
   writes 256 plane pointers from `bitplanes` on, BUG(orig) #777) */
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
 * Searches the BMHD (5120 bytes from source, by 2 bytes) and the BODY
 * (from the BMHD on, BUG(orig) (#766)), then unpacks numlines lines into
 * the planes, each plane pointer + xy_offset.  Fails without BMHD or BODY,
 * if the bitmap has fewer planes or bytes per row than the picture, or no
 * planes.  BUG(orig): every BODY is taken as ByteRun1 (#773), the row
 * width is width/8 (#775), a row counts its bytes in a signed byte (rows
 * of 129 bytes and more are skipped, #774), a run does not stop at the
 * end of the row (#776), nPlanes 0 stores 256 plane pointers (#777).
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
    const UBYTE *chunk = src, *body;
    const struct BitMapHeader *hd;
    ULONG left = 5120;
    UWORD rowbytes;                             /* d7: the BMHD byte width */
    UWORD bodyleft;                             /* d7: the BODY search countdown */
    UWORD line;
    UBYTE planes, code;

    while (LG(chunk) != ID('B', 'M', 'H', 'D')) {
        chunk += 2;
        if ((left -= 2) == 0)
            return BB_FALSE;
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
    if (planes > bm->Depth)                     /* BHI: unsigned bytes */
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
    rowbytes = (UWORD)((WORD)hd->bmh_Width >> 3); /* ASR.w #3 */
    if (rowbytes > v.bitmapwidth)               /* BHI */
        return BB_FALSE;
    v.bytewidth = (UWORD)(rowbytes - 1);
    body = (const UBYTE *)hd;
    bodyleft = 5120;
    while (LG(body) != ID('B', 'O', 'D', 'Y')) {
        body += 2;
        bodyleft -= 2;                          /* SUBQ.w #2:BMI */
        if ((WORD)bodyleft < 0)
            return BB_FALSE;
    }
    body += 8;
    line = v.iffheight;
    do {
        UBYTE **pp = v.bitplanes;
        UWORD plane = v.iffdepth;
        do {
            UBYTE *out = *pp;
            UBYTE cnt = (UBYTE)v.bytewidth;     /* d5.b */
            *pp++ += v.bitmapwidth;
            while (!(cnt & 0x80)) {             /* TST.b d5:BMI */
                UWORD n;
                code = *body++;
                if (code == 0x80)
                    continue;
                if (!(code & 0x80)) {
                    cnt = (UBYTE)(cnt - code - 1);
                    n = code;
                    do
                        *out++ = *body++;
                    while (n-- != 0);
                } else {
                    UBYTE b = *body++;
                    code = (UBYTE)-code;
                    cnt = (UBYTE)(cnt - code - 1);
                    n = code;
                    do
                        *out++ = b;
                    while (n-- != 0);
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
 * Searches the CMAP (5122 bytes from source, by 2 bytes, BUG(orig)
 * (#766)) and writes its colours from colour `offset` on; offset < 0
 * gives the object a new colour block of the CMAP's size.
 * BUG(orig) (#769): without offset, or with an offset into a palette
 * that is too small, the old block is always freed and a new one
 * allocated (the offset ignored); BUG(orig) (#768): the freed size comes
 * from the object's colour count, and on a failed allocation the object
 * keeps the freed block.
 * @param src     d0: the ILBM data
 * @param obj     a1: the palette object (parameter 1)
 * @param offset  d2: the first colour, < 0: a new block
 * @return d0: BB_TRUE, BB_FALSE without CMAP or memory
 */
static BB_BOOL BLITZ_ROUTINE ilbmpalette2_c(const UBYTE *src BL_REG(d0),
                                            struct bb_palette *obj BL_REG(a1),
                                            LONG offset BL_REG(d2))
{
    const UBYTE *cmap = src;
    LONG left = 5120;
    UBYTE *to;
    struct bl_regs r;
    UWORD n;

    while (LG(cmap) != ID('C', 'M', 'A', 'P')) {
        cmap += 2;
        if ((left -= 2) < 0)                    /* SUBQ.l #2:BPL */
            return BB_FALSE;
    }
    cmap += 4;                                  /* the chunk size */
    if (PDATA(obj)) {
        ULONG oldcols = ((ULONG)src & 0xffff0000UL) | obj->numcols;  /* MOVE.w 4(a1),d0 */
        int kill = offset < 0;
        if (!kill) {
            ULONG need = (bl_divu_w(LG(cmap), 3) & 0xffff) + offset;
            kill = (WORD)oldcols < (WORD)need;  /* CMP.w d7,d0:BLT */
        }
        if (!kill)
            to = (UBYTE *)PDATA(obj) + 4;
        else {                                  /* killmem */
            offset = -1;
            r = (struct bl_regs){ .d0 = bl_mulu_w((UWORD)oldcols, 12) + 16, .a1 = PDATA(obj) };
            BLITZ_ALIBJSR(TOK_globfree, r);
            goto reserve;
        }
    } else {
    reserve: {
            ULONG cols;
            UBYTE *block;
            r = (struct bl_regs){ .d0 = bl_mulu_w((UWORD)LG(cmap), 4) + 16,
                                  .d1 = MEMF_PUBLIC | MEMF_CLEAR };
            block = (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
            if (!block)
                return BB_FALSE;
            PDATA(obj) = (ULONG)block;
            cols = bl_divu_w(LG(cmap), 3);
            obj->numcols = (UWORD)cols;
            *(ULONG *)&obj->cyc[0] = 0;
            LG(block) = 0;
            W(block) = (UWORD)cols;
            to = block + 4;
        }
    }
    /* BUG(orig) (#991): for a palette without a colour block the offset is
       not reset (only killmem does it): the block above is sized for the
       CMAP alone and the colours land offset * 12 bytes into it, from
       offset 2 on behind it. */
    if (offset >= 0)                            /* palbig */
        to += bl_mulu_w((UWORD)offset, 12);
    n = (UWORD)(bl_divu_w(LG(cmap), 3) - 1);
    cmap += 4;
    do {
        int i;
        for (i = 0; i < 3; i++) {
            LG(to) = 0;
            *to = *cmap++;
            to += 4;
        }
    } while (n-- != 0);
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
 * The file goes into a free bank of RIAmosFuncLib (NextBank from 51 on,
 * BLoad), UnpackIFF and (with a palette) ILBMPalette from there, then
 * the bank is erased.  BUG(orig) (#771): BB_TRUE also if UnpackIFF
 * failed.
 * @param name  d0: the file name
 * @param bm    a0: the bitmap object (parameter 1)
 * @param pal   a1: the palette object (parameter 2), NULL: none
 * @return d0: BB_TRUE, BB_FALSE if there is no bank or the file does not
 *         load
 */
static BB_BOOL BLITZ_ROUTINE ilbmload_c(ULONG name BL_REG(d0), struct BitMap *bm BL_REG(a0),
                                        struct bb_palette *pal BL_REG(a1))
{
    struct bl_regs r = { .d0 = 51 };
    ULONG bank, start;

    bank = BLITZ_ALIBJSR(TOK_nextbank, r);
    if ((LONG)bank < 0)
        return BB_FALSE;
    if (!BL_JSR_D4_(BL_TOKVAL(TOK_bload, 1), name, bank, 0, 0)) {
        r = (struct bl_regs){ .d0 = bank };
        BLITZ_ALIBJSR(TOK_erase, r);
        return BB_FALSE;
    }
    r = (struct bl_regs){ .d0 = bank };
    start = BLITZ_ALIBJSR(TOK_start, r);
    iff1_c((const UBYTE *)start, bm, 0);
    if (pal) {
        r = (struct bl_regs){ .d0 = bank };
        start = BLITZ_ALIBJSR(TOK_start, r);
        ilbmpalette_c((const UBYTE *)start, pal);
    }
    r = (struct bl_regs){ .d0 = bank };
    BLITZ_ALIBJSR(TOK_erase, r);
    return BB_TRUE;
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
