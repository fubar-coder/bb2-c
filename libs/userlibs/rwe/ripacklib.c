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
void libinit(void), libfinit(void);

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

/* init and finit: RTS */
void BLITZ_ROUTINE libinit(void)
{
}

void BLITZ_ROUTINE libfinit(void)
{
}

/* ChunkHeader(a$): the first long of the string */
static ULONG BLITZ_ROUTINE chunkheader_c(const UBYTE *s BL_REG(d0))
{
    return LG(s);
}
BLITZ_THUNK(chunkheader, chunkheader_c, "d1/a0-a1");

/* ------------------------------------------------------------ UnPackPBM */

/* ByteRun1: n >= 0 copies n+1 bytes, n < 0 (not -128) repeats the next
   byte 1-n times, -128 is skipped.  BUG(orig): the end tests come after
   the access and compare signed - the byte at source+length is read, one
   byte past destlength written (#778). */
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

/* PackPBM(source,dest,length) - ByteRun1 with runs of at most 128 (127
   in the counter); -1 if the result would reach `length` bytes.  d3 is
   only used by the failure (BUG(orig): UnPackPBM, see above, #772). */
static ULONG BLITZ_ROUTINE packpbm_c(const UBYTE *src BL_REG(d0), UBYTE *dst BL_REG(d1),
                                     ULONG size BL_REG(d2), ULONG d3 BL_REG(d3))
{
    const UBYTE *a0 = src, *a5, *end = src + size;
    UBYTE *a1 = dst, *limit = dst + size;
    UBYTE d0, d1, prev;
    UWORD n;

#define PUT(v) do { *a1++ = (v); if (a1 == limit) goto failure; } while (0)
    for (;;) {
        if (a0 == end)                          /* check1 */
            return a1 - dst;
        a5 = a0;
        d0 = *a5++;
        if (a5 == end) {                        /* finish1: one byte */
            PUT(0);
            PUT(d0);
            return a1 - dst;
        }
        if (*a5 != d0) {                        /* literals */
            int last = 0;
            d1 = 0;
            for (;;) {                          /* check3 */
                prev = d0;
                d0 = *a5++;
                if (d0 == prev)
                    break;
                d1++;
                if (a5 == end) {
                    last = 1;                   /* check41 */
                    break;
                }
                if (d1 == 127)
                    break;
            }
            PUT(d1);
            n = d1;
            do
                PUT(*a0++);
            while (n-- != 0);
            if (last)
                return a1 - dst;
        } else {                                /* check5: a run */
            int last = 0;
            d1 = 0;
            a0 = a5;
            for (;;) {
                d0 = *a0++;
                if (d0 != *a5) {
                    a0--;                       /* check62 */
                    break;
                }
                d1++;
                if (a0 == end) {
                    last = 1;                   /* check61 */
                    break;
                }
                if (d1 == 127)
                    break;
            }
            PUT((UBYTE)-d1);
            PUT(*a5);
            if (last)
                return a1 - dst;
        }
    }
failure:
    /* BUG(orig): no RTS - into _UnPackPBM with d0 = -1 (#772) */
    unpackpbm_c((const UBYTE *)-1, dst, size, d3);
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
    UWORD d1 = 0;

    do
        d1 = (UWORD)(d1 << 1 | ice_bit(s));
    while (n-- != 0);
    return d1;
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
            ULONG d1 = 0;
            UWORD n;

            if (ice_bit(s)) {                   /* the length of the run */
                const ULONG *t = direkt_tab + 5;
                WORD d3 = 4;
                for (;;) {
                    ULONG d0 = *--t;
                    d1 = ice_bits(s, (UWORD)d0);
                    if ((UWORD)d1 != (UWORD)(d0 >> 16))     /* DBNE */
                        break;
                    if (d3-- == 0)
                        break;
                }
                d1 += t[5];                     /* ADD.l 20(a1),d1 */
            }
            n = (UWORD)d1;
            do                                  /* copy_direkt */
                *--s->dst = *--s->src;
            while (n-- != 0);
        }
        if (!((LONG)s->dst > (LONG)s->start))  /* test_if_end: BGT */
            return;
        {                                       /* strings */
            WORD d2 = 3, d0, e;
            UWORD d1 = 0, d4, n;
            const UBYTE *from;

            while (ice_bit(s) && d2-- != 0)     /* DBCC */
                ;
            d0 = length_tab[1 + d2];
            if (d0 >= 0)
                d1 = ice_bits(s, (UWORD)d0);
            d4 = (UWORD)((UBYTE)length_tab[6 + d2] + d1);
            if (d4) {
                e = 1;
                while (ice_bit(s) && e-- != 0)
                    ;
                d1 = ice_bits(s, (UWORD)more_offset[1 + e]);
                d1 = (UWORD)(d1 + more_add[1 + e]);
                if ((WORD)d1 < 0)               /* BPL / SUB.w d4,d1 */
                    d1 = (UWORD)(d1 - d4);
            } else {                            /* get_offset_2 */
                WORD add = -1;
                UWORD bits = 5;
                if (ice_bit(s)) {
                    bits = 8;
                    add = 0x3f;
                }
                d1 = (UWORD)(ice_bits(s, bits) + add);
            }
            from = s->dst + 2 + (WORD)d4 + (WORD)d1;    /* depack_bytes */
            *--s->dst = *--from;
            n = d4;
            do
                *--s->dst = *--from;
            while (n-- != 0);
        }
    }
}

/* DeIce(source,dest): -1 after unpacking "ICE!" data, else 0 */
/* getinfo: a long read byte by byte (the data may be at an odd address) */
static ULONG getinfo(const UBYTE *p)
{
    return (ULONG)p[0] << 24 | (ULONG)p[1] << 16 | (ULONG)p[2] << 8 | p[3];
}

static ULONG BLITZ_ROUTINE deice_c(const UBYTE *src BL_REG(d0), UBYTE *dst BL_REG(d1))
{
    struct ice s;
    UBYTE *end;
    UWORD count;
    UWORD *p;

    if (getinfo(src) != ID('I', 'C', 'E', '!'))
        return 0;
    s.src = src + getinfo(src + 4);             /* LEA -8(a0,d0.l),a5 */
    end = dst + getinfo(src + 8);
    s.dst = end;
    s.start = dst;
    s.bits = *--s.src;                          /* MOVE.b -(a5),d7 */
    ice_normal(&s);
    /* the picture flag: Atari ST low-res bitplanes -> interleaved words */
    if (!ice_bit(&s))
        return -1;
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
    return -1;
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

/* UnpackIFF(source,bitmap#,numlines,xy_offset): -1, or 0 without
   BMHD/BODY or if the bitmap is too small (a1 = the bitmap object; lines
   < 0: all) */
static ULONG BLITZ_ROUTINE unpackiff_c(const UBYTE *src BL_REG(d0), UBYTE *bm BL_REG(a1),
                                       ULONG lines BL_REG(d2), ULONG xy BL_REG(d3))
{
    const UBYTE *a2 = src, *body;
    const UBYTE *hd;
    ULONG left = 5120;
    UWORD d7, line;
    UBYTE planes, d0;

    while (LG(a2) != ID('B', 'M', 'H', 'D')) {
        a2 += 2;
        if ((left -= 2) == 0)
            return 0;
    }
    hd = a2 + 8;
    if ((WORD)lines < 0)
        BL_SETW(lines, W(hd + 2));              /* the height of the BMHD */
    if ((WORD)((UWORD)lines - W(bm + 2)) >= 0)  /* CMP.w 2(a1),d2:BMI */
        BL_SETW(lines, W(bm + 2));
    BL_SETW(lines, lines - 1);
    if ((WORD)lines < 0)
        return 0;
    v.iffheight = (UWORD)lines;
    planes = hd[8];
    if (planes > bm[5])                         /* BHI: unsigned bytes */
        return 0;
    v.iffdepth = (UBYTE)(planes - 1);
    v.bitmapwidth = W(bm);
    if (!LG(bm + 8))
        return 0;
    {
        const ULONG *pl = (const ULONG *)(bm + 8);
        UBYTE **to = v.bitplanes;
        UWORD n = v.iffdepth;
        do
            *to++ = (UBYTE *)(*pl++ + xy);
        while (n-- != 0);
    }
    d7 = (UWORD)((WORD)W(hd) >> 3);             /* ASR.w #3 */
    if (d7 > v.bitmapwidth)                     /* BHI */
        return 0;
    v.bytewidth = (UWORD)(d7 - 1);
    body = hd;
    d7 = 5120;
    while (LG(body) != ID('B', 'O', 'D', 'Y')) {
        body += 2;
        d7 -= 2;                                /* SUBQ.w #2:BMI */
        if ((WORD)d7 < 0)
            return 0;
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
                d0 = *body++;
                if (d0 == 0x80)
                    continue;
                if (!(d0 & 0x80)) {
                    cnt = (UBYTE)(cnt - d0 - 1);
                    n = d0;
                    do
                        *out++ = *body++;
                    while (n-- != 0);
                } else {
                    UBYTE b = *body++;
                    d0 = (UBYTE)-d0;
                    cnt = (UBYTE)(cnt - d0 - 1);
                    n = d0;
                    do
                        *out++ = b;
                    while (n-- != 0);
                }
            }
        } while (plane-- != 0);
    } while (line-- != 0);
    return -1;
}
BLITZ_THUNK(unpackiff, unpackiff_c, "d1/a0-a1");

/* UnpackIFF(source,bitmap#): MOVE.w #-1,d2:MOVEQ #0,d3 */
static ULONG BLITZ_ROUTINE iff1_c(const UBYTE *src BL_REG(d0), UBYTE *bm BL_REG(a1),
                                  ULONG d2 BL_REG(d2))
{
    BL_SETW(d2, 0xffff);
    return unpackiff_c(src, bm, d2, 0);
}
BLITZ_THUNK(iff1, iff1_c, "d1/a0-a1");

/* ---------------------------------------------------------- ILBMPalette */

/* ILBMPalette(source,palette#,offset): the CMAP into the palette object
   (a1) from colour `offset` on (< 0: a new block, see the BUG(orig) at
   the top, #768, #769); -1, or 0 without CMAP or memory */
static ULONG BLITZ_ROUTINE ilbmpalette2_c(const UBYTE *src BL_REG(d0), UBYTE *obj BL_REG(a1),
                                          LONG offset BL_REG(d2))
{
    const UBYTE *a0 = src;
    LONG left = 5120;
    UBYTE *to;
    struct bl_regs r;
    UWORD n;

    while (LG(a0) != ID('C', 'M', 'A', 'P')) {
        a0 += 2;
        if ((left -= 2) < 0)                    /* SUBQ.l #2:BPL */
            return 0;
    }
    a0 += 4;                                    /* the chunk size */
    if (LG(obj)) {
        ULONG d0 = ((ULONG)src & 0xffff0000UL) | W(obj + 4);   /* MOVE.w 4(a1),d0 */
        int kill = offset < 0;
        if (!kill) {
            ULONG need = (bl_divu_w(LG(a0), 3) & 0xffff) + offset;
            kill = (WORD)d0 < (WORD)need;       /* CMP.w d7,d0:BLT */
        }
        if (!kill)
            to = *(UBYTE **)obj + 4;
        else {                                  /* killmem */
            offset = -1;
            r = (struct bl_regs){ .d0 = bl_mulu_w((UWORD)d0, 12) + 16, .a1 = LG(obj) };
            BLITZ_ALIBJSR(TOK_globfree, r);
            goto reserve;
        }
    } else {
    reserve: {
            ULONG cols;
            UBYTE *block;
            r = (struct bl_regs){ .d0 = bl_mulu_w((UWORD)LG(a0), 4) + 16,
                                  .d1 = MEMF_PUBLIC | MEMF_CLEAR };
            block = (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
            if (!block)
                return 0;
            LG(obj) = (ULONG)block;
            cols = bl_divu_w(LG(a0), 3);
            W(obj + 4) = (UWORD)cols;
            LG(obj + 6) = 0;
            LG(block) = 0;
            W(block) = (UWORD)cols;
            to = block + 4;
        }
    }
    if (offset >= 0)                            /* palbig */
        to += bl_mulu_w((UWORD)offset, 12);
    n = (UWORD)(bl_divu_w(LG(a0), 3) - 1);
    a0 += 4;
    do {
        int i;
        for (i = 0; i < 3; i++) {
            LG(to) = 0;
            *to = *a0++;
            to += 4;
        }
    } while (n-- != 0);
    return -1;
}
BLITZ_THUNK(ilbmpalette2, ilbmpalette2_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE ilbmpalette_c(const UBYTE *src BL_REG(d0), UBYTE *obj BL_REG(a1))
{
    return ilbmpalette2_c(src, obj, -1);
}
BLITZ_THUNK(ilbmpalette, ilbmpalette_c, "d1/a0-a1");

/* ILBMGrab source,bitmap#,palette#: UnpackIFF and ILBMPalette */
static void BLITZ_ROUTINE ilbmgrab_c(const UBYTE *src BL_REG(d0), UBYTE *bm BL_REG(a0),
                                     UBYTE *pal BL_REG(a1), ULONG d2 BL_REG(d2))
{
    iff1_c(src, bm, d2);
    ilbmpalette_c(src, pal);
}
BLITZ_THUNK(ilbmgrab, ilbmgrab_c, "d0-d1/a0-a1");

/* -------------------------------------------------------------- LoadIFF */

/* LoadIFF(filename$,bitmap[,palette]): the file into a free bank of
   RIAmosFuncLib (from 51 on), UnpackIFF and ILBMPalette from there, the
   bank erased; -1 (BUG(orig): also if UnpackIFF failed, #771), 0 if there is
   no bank or the file does not load */
static ULONG BLITZ_ROUTINE ilbmload_c(ULONG name BL_REG(d0), UBYTE *bm BL_REG(a0),
                                      UBYTE *pal BL_REG(a1))
{
    struct bl_regs r = { .d0 = 51 };
    ULONG bank, start;

    bank = BLITZ_ALIBJSR(TOK_nextbank, r);
    if ((LONG)bank < 0)
        return 0;
    if (!BL_JSR_D4_(BL_TOKVAL(TOK_bload, 1), name, bank, 0, 0)) {
        r = (struct bl_regs){ .d0 = bank };
        BLITZ_ALIBJSR(TOK_erase, r);
        return 0;
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
    return -1;
}
BLITZ_THUNK(ilbmload, ilbmload_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE ilbmloadnopal_c(ULONG name BL_REG(d0), UBYTE *bm BL_REG(a0))
{
    return ilbmload_c(name, bm, NULL);
}
BLITZ_THUNK(ilbmloadnopal, ilbmloadnopal_c, "d1/a0-a1");
