/*
 * blitzobj.h - the Blitz objects that several libraries share.
 *
 * A library that owns an object type (the !libfin item size) allocates
 * the object table; other libraries get an object through !libs (a
 * register with #item / #used) and read or write its fields.  The
 * originals addressed the fields with offsets; here every shared object
 * has one struct, laid out as in the owning library's source, with the
 * owning library's member names.  Where readers take two fields as one
 * word or long (the original's MOVE.w / MOVE.l), an anonymous union gives
 * that view too.
 *
 * The structs are exactly the bytes of the object (`static_assert` on
 * the size and the key offsets).  m68k aligns words and longs to 2, so
 * there is no padding; BL_PACKED (also aligned 2) does not change the
 * layout and is kept where the owning library had it.
 *
 * Include it after blitzlib.h.
 */
#ifndef BLITZOBJ_H
#define BLITZOBJ_H

#include <stddef.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <graphics/text.h>

#ifndef BL_PACKED
#define BL_PACKED __attribute__((packed, aligned(2)))
#endif

/* ------------------------------------------------------------- shape */

/** "cL" in bb_shape::cludge and bb_sound::mark: the data is not the
    object's own (CludgeShape(s), DecodeShape(s), DecodeSound) */
#define BB_CLUDGE 0x634c

/**
 * @brief A Shape object, 32 bytes (shapeslib's; also the block
 * SaveShape(s) writes and the header of a shape for CludgeShapes).
 *
 * Owner: shapeslib.  Read or written by shapetrixlib, bitmaplib, blitlib,
 * bblitlib, 2dlib, qblitlib, stencillib, collslib, spriteslib, iffmakelib,
 * gtlib, gadgetslib, menuslib, windowslib and the RWE libraries
 * rishapeslib, rimapeditorlib, ritooltypeslib, rimapshapeslib,
 * riwbspritelib, bb_blitlib.
 */
struct bb_shape {
    UWORD width;                /**< +0 pixel width, 0 = no shape */
    UWORD height;               /**< +2 pixel height */
    UWORD depth;                /**< +4 number of planes */
    UWORD ebwidth;              /**< +6 even byte width (bytes per row) */
    UWORD bltsize;              /**< +8 height << 6 | words + 1 */
    WORD handlex;               /**< +10 handle x */
    WORD handley;               /**< +12 handle y */
    UBYTE *data;                /**< +14 plane after plane, CHIP */
    UBYTE *cookie;              /**< +18 OR of all planes, CHIP, or 0 */
    UWORD planesize;            /**< +22 bytes per plane */
    UWORD xplanesize;           /**< +24 per plane + one word per row */
    UWORD size;                 /**< +26 the whole shape */
    UWORD xsize;                /**< +28 whole + one word per row */
    UWORD cludge;               /**< +30 BB_CLUDGE: the data is not ours */
};
static_assert(sizeof(struct bb_shape) == 32, "bb_shape size");
static_assert(offsetof(struct bb_shape, data) == 14, "bb_shape data");
static_assert(offsetof(struct bb_shape, planesize) == 22, "bb_shape planesize");
static_assert(offsetof(struct bb_shape, cludge) == 30, "bb_shape cludge");

/* ------------------------------------------------------------ bitmap */

/**
 * @brief A BitMap object, 64 bytes: a struct BitMap (+0 .. +39) and the
 * Blitz fields behind it.
 *
 * Owner: bitmaplib (BitMap, CludgeBitMap, interleaved bitmaps); 2dlib sets
 * the origin +48/+50 (BitMapOrigin) and, for BitMapWindow, bit 0 of the
 * word at +52 and the window's byte width at +54; fontlib keeps the text
 * cursor at +40/+42.  Read by shapeslib, 2dlib, polylib, scrolllib,
 * blitlib, bblitlib, qblitlib, stencillib, animlib, blitzcoplib,
 * screenslib, fontlib, blitzoslib and the RWE libraries bb_blitlib,
 * rigfxlib.  Several readers take Flags/Depth (+4) as one word
 * (flagsdepth) and the interleave flag as a word (ilword).  displaylib
 * walks the object with a byte pointer; ilbmifflib, iffmakelib,
 * windowslib and several RWE libraries read only the struct BitMap part,
 * as a struct BitMap.
 */
struct bb_bitmap {
    UWORD bpr;                  /**< +0 BytesPerRow */
    UWORD rows;                 /**< +2 Rows */
    union {
        struct {
            UBYTE flags;        /**< +4 Flags */
            UBYTE depth;        /**< +5 Depth */
        };
        UWORD flagsdepth;       /**< +4 Flags << 8 | Depth, as one word */
    };
    UWORD oldfd;                /**< +6 Flags/Depth before interleaving */
    UBYTE *planes[8];           /**< +8 Planes */
    UWORD cursx;                /**< +40 text cursor x in pixels (fontlib) */
    UWORD cursy;                /**< +42 text cursor y in pixels (fontlib) */
    UBYTE pad44[4];             /**< +44 */
    WORD xorigin;               /**< +48 BitMapOrigin x (2dlib) */
    WORD yorigin;               /**< +50 BitMapOrigin y (2dlib) */
    union {
        struct {
            UBYTE ilflag;       /**< +52 bit 7: interleaved */
            UBYTE pad53;        /**< +53 */
        };
        UWORD ilword;           /**< +52 as one word: negative = interleaved */
    };
    UWORD planedist;            /**< +54 bytes from one plane's row to the
                                     next plane's (interleaved) */
    UWORD width;                /**< +56 clip width */
    UWORD height;               /**< +58 clip height */
    UWORD colours;              /**< +60 */
    WORD isreal;                /**< +62 0 = not set up, 1 = borrowed
                                     planes, -1 = own planes */
} BL_PACKED;
static_assert(sizeof(struct bb_bitmap) == 64, "bb_bitmap size");
static_assert(offsetof(struct bb_bitmap, planes) == 8, "bb_bitmap planes");
static_assert(offsetof(struct bb_bitmap, xorigin) == 48, "bb_bitmap origin");
static_assert(offsetof(struct bb_bitmap, ilflag) == 52, "bb_bitmap ilflag");
static_assert(offsetof(struct bb_bitmap, width) == 56, "bb_bitmap width");
static_assert(offsetof(struct bb_bitmap, isreal) == 62, "bb_bitmap isreal");

/* ----------------------------------------------------------- palette */

/** @brief One colour cycle of a palette object (SetCycle). */
struct bb_cycle {
    WORD lo;                    /**< +0 first colour */
    WORD hi;                    /**< +2 last colour */
    WORD speed;                 /**< +4 */
    WORD var;                   /**< +6 the counter */
};

/**
 * @brief A Palette object, 128 bytes.
 *
 * Owner: palettelib.  The colour block pdata is LoadRGB32's table: the
 * number of colours (word), the first colour (word), then 3 longs per
 * colour (each channel in the top byte), a 0 long at the end.  Read by
 * shapetrixlib, fadelib, iffmakelib (pdata and numcols only) and the RWE
 * libraries rishapeslib, rigfxlib, ribbmaplib, ripacklib, ridatatypeslib.
 */
struct bb_palette {
    UBYTE *pdata;               /**< +0 the colour block, 0 = none */
    UWORD numcols;              /**< +4 number of colours */
    struct bb_cycle cyc[14];    /**< +6 the colour cycles */
    UBYTE pad[10];              /**< +118 */
} BL_PACKED;
static_assert(sizeof(struct bb_palette) == 128, "bb_palette size");
static_assert(offsetof(struct bb_palette, numcols) == 4, "bb_palette numcols");

/* ------------------------------------------------------------- sound */

/**
 * @brief A Sound object, 16 bytes.
 *
 * Owner: audiolib.  Read by the RWE libraries risoundlib, risoundlib2.
 */
struct bb_sound {
    UBYTE *data;                /**< +0 the sample (CHIP) */
    UWORD period;               /**< +4 */
    UWORD len;                  /**< +6 length in words */
    UBYTE *rep;                 /**< +8 the loop */
    UWORD replen;               /**< +12 loop length in words */
    UWORD mark;                 /**< +14 BB_CLUDGE: DecodeSound data */
};
static_assert(sizeof(struct bb_sound) == 16, "bb_sound size");
static_assert(offsetof(struct bb_sound, rep) == 8, "bb_sound rep");

/* ------------------------------------------------------------- slice */

/**
 * @brief A Slice object, 64 bytes (Blitz mode).  The pointers are
 * addresses in the copper list (0 = not there).
 *
 * Owner: blitzcoplib.  Read by spriteslib (the sprite moves, the height,
 * BPLCON2), mouselib, fadelib and palettelib (the colour moves).
 */
struct bb_slice {
    UWORD y;                    /**< +0 0 = not in use */
    union {                     /* +2 */
        UWORD flags;            /**< +2 the slice flags */
        struct { UBYTE flagshi, flagslo; };
    };
    union {                     /* +4 bits 14/15: dual playfield half */
        UWORD planes;           /**< +4 number of bitplanes */
        struct { UBYTE planeshi, planeslo; };
    };
    UWORD sprites;              /**< +6 number of sprites */
    UWORD colours;              /**< +8 number of colours */
    union {                     /* +10 */
        ULONG moves[5];
        struct {
            ULONG bplmoves;     /**< +10 the bitplane pointer moves */
            ULONG sprmoves;     /**< +14 the sprite pointer moves */
            ULONG colmoves;     /**< +18 the colour moves */
            ULONG con1;         /**< +22 the BPLCON1 value */
            ULONG con2;         /**< +26 the BPLCON2 value */
        };
    };
    UWORD con1val;              /**< +30 a copy of BPLCON1 */
    UWORD height;               /**< +32 */
    UBYTE pad[30];              /**< +34 */
} BL_PACKED;
static_assert(sizeof(struct bb_slice) == 64, "bb_slice size");
static_assert(offsetof(struct bb_slice, sprmoves) == 14, "bb_slice sprmoves");
static_assert(offsetof(struct bb_slice, height) == 32, "bb_slice height");

/* ----------------------------------------------------------- coplist */

/**
 * @brief A CopList object, NEWTYPE .mycop, 128 bytes (the display
 * library's copper list of a display).
 *
 * Owner: displaylib.  Read by polylib (the chunky area) and the RWE
 * library ricopperfxlib (the custom space).
 */
struct bb_coplist {
    ULONG size;                 /**< +0 0 = not initialised */
    ULONG coppos;               /**< +4 the list (chip memory) */
    ULONG colors;               /**< +8 pointers into the list */
    ULONG sprites;              /**< +12 */
    ULONG bpcons;               /**< +16 */
    ULONG bplanes;              /**< +20 */
    ULONG dot;                  /**< +24 */
    ULONG customs;              /**< +28 */
    ULONG dob;                  /**< +32 */
    UWORD bot;                  /**< +36 */
    UWORD numbp;                /**< +38 */
    UWORD colpokes;             /**< +40 */
    UWORD fetchwid;             /**< +42 */
    UWORD xand;                 /**< +44 */
    UWORD xshift;               /**< +46 */
    UWORD ypos;                 /**< +48 */
    UWORD height;               /**< +50 */
    UWORD res;                  /**< +52 */
    UWORD numsprites;           /**< +54 */
    UWORD numcols;              /**< +56 */
    UWORD numcustoms;           /**< +58 */
    UWORD cpline;               /**< +60 */
    ULONG x0;                   /**< +62 .q */
    ULONG x1;                   /**< +66 .q */
    UWORD bplcon3;              /**< +70 */
    UWORD aga;                  /**< +72 $8000 24 bit, fetch $00-$c0 in the low byte */
    UWORD resshift;             /**< +74 */
    UWORD setup;                /**< +76 */
    UWORD cblow;                /**< +78 */
    UWORD sfetch;               /**< +80 */
    UWORD spres;                /**< +82 */
    UWORD spif;                 /**< +84 */
    UWORD spwid;                /**< +86 */
    UWORD sspwid;               /**< +88 */
    UWORD ham8;                 /**< +90 */
    ULONG csprites;             /**< +92 */
    ULONG cchunky;              /**< +96 location of the copper chunky */
    UWORD chy;                  /**< +100 chunky y position */
    UWORD chw;                  /**< +102 chunky width */
    UWORD chh;                  /**< +104 chunky height */
    UWORD chr;                  /**< +106 chunky resolution */
    UWORD chc;                  /**< +108 bytes per chunky line */
    UWORD pad0;                 /**< +110 */
    ULONG cchunky2;             /**< +112 the same for the split part */
    UWORD chy2, chw2, chh2, chr2, chc2;         /**< +116 */
    UWORD spri;                 /**< +126 the sprite resolution shift */
} BL_PACKED;
static_assert(sizeof(struct bb_coplist) == 128, "bb_coplist size");
static_assert(offsetof(struct bb_coplist, customs) == 28, "bb_coplist customs");
static_assert(offsetof(struct bb_coplist, x0) == 62, "bb_coplist x0");
static_assert(offsetof(struct bb_coplist, cchunky) == 96, "bb_coplist cchunky");
static_assert(offsetof(struct bb_coplist, chy2) == 116, "bb_coplist chy2");
static_assert(offsetof(struct bb_coplist, spri) == 126, "bb_coplist spri");

/* ------------------------------------------------------------ sprite */

/**
 * @brief A Sprite object, 16 bytes.
 *
 * Owner: spriteslib.  Read by displaylib (DisplaySprite), mouselib and
 * collslib.
 */
struct bb_sprite {
    UBYTE *data;                /**< +0 the images (CHIP), 0 = none */
    UWORD height;               /**< +4 height + 1 (for the final zeros) */
    UWORD count;                /**< +6 number of hardware sprites used */
    union {
        UWORD flags;            /**< +8 bit 15 = 16 colours (attached pairs) */
        struct {
            UBYTE attach;       /**< +8 bit 7 = 16 colours */
            UBYTE width;        /**< +9 pixel width */
        };
    };
    UWORD imgsize;              /**< +10 bytes per image */
    UWORD handlex;              /**< +12 handle x */
    UWORD handley;              /**< +14 handle y */
};
static_assert(sizeof(struct bb_sprite) == 16, "bb_sprite size");
static_assert(offsetof(struct bb_sprite, imgsize) == 10, "bb_sprite imgsize");

/* --------------------------------------------------------- IntuiFont */

/**
 * @brief An IntuiFont object, 16 bytes: a TextAttr, the opened font.
 *
 * Owner: intuifontlib.  Read by windowslib (WindowFont) and menuslib (its
 * error checks); menuslib and gadgetslib take the TextAttr at its start
 * through intuifontlib's pointer as a struct TextAttr.
 */
struct bb_intuifont {
    struct TextAttr ta;         /**< +0 name, +4 ysize, +6 style, +7 flags */
    struct TextFont *font;      /**< +8 OpenDiskFont's result */
    ULONG pad;                  /**< +12 */
};
static_assert(sizeof(struct bb_intuifont) == 16, "bb_intuifont size");
static_assert(offsetof(struct bb_intuifont, font) == 8, "bb_intuifont font");

/* ----------------------------------------------------- string memory */

/**
 * @brief The memory of a string variable: the variable points to
 * `chars`, the header is in front of it (-8, -4).
 *
 * Owner: the compiler's runtime (memlib allocates it).  Used by maxlenlib
 * and strings2lib.
 */
struct bb_strmem {
    ULONG alloced;              /**< -8 alloc'd length */
    ULONG len;                  /**< -4 string length */
    UBYTE chars[];              /**< +0 the characters, then a 0 */
};
static_assert(sizeof(struct bb_strmem) == 8, "bb_strmem size");

/* ------------------------------------------------------------- lists */

/** "lIsT" in bb_listhead::id */
#define BB_LISTID 0x6c497354UL

/** @brief The node at the start of every item of a Blitz list. */
struct bb_listitem {
    struct bb_listitem *next;   /**< +0 0 in the node that ends the list */
    struct bb_listitem *prev;   /**< +4 */
    UBYTE data[];               /**< +8 the item's fields */
};
static_assert(sizeof(struct bb_listitem) == 8, "bb_listitem size");

/**
 * @brief The 32 bytes in front of a Blitz list array (Dim List): the
 * array points behind it, so the fields are at -32 .. -4.
 *
 * Owner: arrayslib (newlist).  Used by linklistlib and the RWE libraries
 * risortlib, riamosfunclib.
 */
struct bb_listhead {
    struct bb_listitem *current;        /**< -32 the current item */
    struct MinList used;                /**< -28 the items in use */
    struct MinList free;                /**< -16 the free items */
    ULONG id;                           /**< -4 BB_LISTID */
};
static_assert(sizeof(struct bb_listhead) == 32, "bb_listhead size");
static_assert(offsetof(struct bb_listhead, free) == 16, "bb_listhead free");

#endif /* BLITZOBJ_H */
