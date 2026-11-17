/*
 * blitzlib.h - write Blitz Basic 2 libraries in C.
 *
 * A BB2 library is an AmigaDOS executable with one code hunk that starts
 * with a header and a table of token entries, followed by the code.  The
 * original libraries were Blitz programs: the macros of libmacs.res
 * (bb2/cd/extras/blitzlibs/libmacs.bb) emitted header and table with `Dc`
 * statements in front of the inline assembler code.
 *
 * The macros below emit exactly the same bytes as the libmacs macro of the
 * same name, as static data in the section ".blitzlib.head".  The linker
 * script ld/blitzlib.ld puts that section at offset 0 of the single code
 * hunk, followed by the code and data and, last, the debug-only error check
 * routines (BLITZ_ERRCHK).  Code addresses in the table (`Dc.l label` in
 * the original) are relocated longs, as in the original.
 *
 * The objects must stay in definition order: compile with
 * -fno-toplevel-reorder (the CMake function blitz_library() does that).
 *
 *   libmacs                         here
 *   !syslibheader{n,f,l,i,r,fi,e,fl} BLITZ_SYSLIBHEADER(n,f,l,i,r,fi,e,fl)
 *   #ldivlib (libnums.bb)           LIB_ldivlib (libnums.h)
 *   !libheader{n,init,ret,finit,err} BLITZ_LIBHEADER(n,init,ret,finit,err)
 *   !astatement                     BLITZ_ASTATEMENT()
 *   !afunction{type}                BLITZ_AFUNCTION(type)
 *   !acommand{type}                 BLITZ_ACOMMAND(type)
 *   !ustatement / !ufunction        BLITZ_USTATEMENT() / BLITZ_UFUNCTION()
 *   !args{t,...}                    BLITZ_ARGS(t,...)   (no args: BLITZ_ARGS0())
 *   !repargs{first,num,t,...}       BLITZ_REPARGS(first,num,t,...)
 *   !libs{lib,reg,...}              BLITZ_LIBS(lib,reg,...) (none: BLITZ_LIBS0())
 *   !subs{code,err1,err2}           BLITZ_SUBS(code,err1,err2)
 *   !name{"name","help"}            BLITZ_NAME("name","help")
 *   !nullsub{code,err1,err2[,libs]} BLITZ_NULLSUB(code,err1,err2[,lib,reg...])
 *   !dumtoke{"name","help",label}   BLITZ_DUMTOKE(label,"name","help")
 *   !libfin / !libfin{t,l,s,u,f,m,s} BLITZ_LIBFIN() / BLITZ_LIBFIN7(...)
 *
 * Library routines take their arguments in registers (args in d0-d5, the
 * values requested with BLITZ_LIBS in the given registers) and return d0;
 * declare them with GCC register parameters:
 *
 *   LONG BLITZ_ROUTINE ldivide(LONG a BL_REG(d0), LONG b BL_REG(d1));
 *
 * a4 (locals) and a5 (globals) are fixed registers of the Blitz program;
 * the build uses -ffixed-a4 -ffixed-a5, so C code can use them through the
 * global register variables below.
 *
 * Small shared helpers (register arithmetic, a2 argument stack, mathffp
 * LVOs, tokens of system lib routines) are in blitzhelp.h, included at the
 * end of this file.
 */
#ifndef BLITZLIB_H
#define BLITZLIB_H

/* Library code has no C library: no memset/memcpy behind the scenes.
   - built with -ffreestanding (checked here),
   - loops must not become memset/memcpy calls (also set by CMake),
   - linked with -nostdlib, so a call GCC still emits anyway (it may for
     large struct copies) is a link error, never a hidden dependency. */
/* BL_CODE_MODEL: the file is read by an IDE's code model, not compiled by
   GCC - clangd (__clang__) and CLion, whose clangd engine takes GCC's
   predefined macros (no __clang__) and defines __JETBRAINS_IDE__.  The
   GCC-only constructs (register parameters, global register variables,
   asm with operands clang cannot check) are left out there. */
#if defined(__clang__) || defined(__JETBRAINS_IDE__)
#define BL_CODE_MODEL 1
#endif

#if !defined(BL_CODE_MODEL) && __STDC_HOSTED__
#error "compile Blitz libraries with -ffreestanding (see CMakeLists.txt)"
#endif
#ifndef BL_CODE_MODEL
#pragma GCC optimize ("no-tree-loop-distribute-patterns")
#endif

#include <exec/types.h>
#include <exec/memory.h>
#include <libnums.h>     /* LIB_xxx = #xxx of libnums.bb; <>: a variant
                             (libs/variants) brings its own */

/* a truth value as Blitz programs see it: -1 true, 0 false (exec's TRUE
   is 1).  Exactly a ULONG (C23 fixed underlying type): d0, BL_REG and the
   code stay the same.  gcc -Wc++-compat and clang -Wassign-enum warn
   about an int assigned to it; an enum used as a ULONG is not caught */
typedef enum : ULONG { BB_FALSE = 0, BB_TRUE = 0xffffffffUL } BB_BOOL;
static_assert(sizeof(BB_BOOL) == sizeof(ULONG), "BB_BOOL must be a long");

/* ----------------------------------------------------------- constants */

/* types (!args / !afunction) */
#define BL_BYTE     1
#define BL_WORD     2
#define BL_LONG     3
#define BL_QUICK    4
#define BL_FLOAT    5           /* Motorola FFP, not IEEE */
#define BL_STRING   7
/* argument flags */
#define BL_USESIZE  0
#define BL_UNKNOWN  8
#define BL_ARRAYEND 16
#define BL_ARRAY    32
#define BL_PUSH     64
#define BL_VARPTR   128

/* init return (!libheader `return`) */
#define BL_WORDRET  (-1)
#define BL_NORET    0
#define BL_LONGRET  1

/* Register codes for BLITZ_LIBS / BLITZ_NULLSUB (!libs{lib,code,...}):
   what the compiler loads from library `lib` into a register before the
   call.  The names are the constants of the original's
   archive/cd/ultimate-blitz-basic-2.1/EXTRAS/BLITZLIBS/LIBMACS.BB with
   the prefix BL_ in capitals (#ld0 = BL_LD0, #ia0 = BL_IA0, #pd0 =
   BL_PD0 ...).  A code is a word, register | kind:
     high byte  the register: $00-$07 d0-d7 (#ld0..#ld7), $10-$13 and $16
                a0-a3 and a6 (#la0..#la3, #la6); $FF00 = pushed onto the
                stack (#lpush)
     low byte   the kind:
       0        the library's variable (its init result, e.g. the library
                base): #ld*, #la*
       1        #base: the base item (item 0) of an object library's block
                of objects: #bd* = #ld*|#base, #ba*
       2        #used: the currently used object: #ud*, #ua*
       3        #max: the maximum setting of the compiler options: #md*,
                #ma*
       $80|n    #item: the object whose number is in dn (#pd0..#pd7 = n);
                the pointer ends in an address register: #ia*|#pd*
   Not in LIBMACS.BB, from the compiler (blitz2.s:11647 toreg; the
   original sources write them in hex):
       4        BL_USEDOFF: the a5 offset of the pointer to the used object
                (move.w #offset,dn; $0604 = d6)
     $8000      BL_IFBLITZ: loaded only in Blitz mode
     $4000      BL_IFAMIGA: loaded only in Amiga mode ($5600 = a6)
   E.g. $1080 = BL_IA0|BL_PD0 (a0 = the object of the number in d0),
   $1201 = BL_BA2, $0303 = BL_MD3, $5600 = BL_IFAMIGA|BL_LA6.

   The letters of the names (LIBMACS.BB):
     L  Library variable: the value the library's init returned (#ld*, #la*)
     B  Base: object 0 of the library's object block (#bd*, #ba*)
     U  Used: the current object, the one `Use` selected (#ud*, #ua*)
     M  Max: the maximum of the compiler options, as a number (#md*, #ma*)
     I  Item: the object whose number is an argument (#ia*), always
        combined with
     PD Parameter in D: the data register that holds that number
        (#pd0..#pd7)
   then D or A and the register number: BL_IA0|BL_PD0 = "item into a0,
   its number from d0".

   Object libraries.  B, U, M and I work only for a library that ends in
   BLITZ_LIBFIN7(toke, load, save, use, free, defmax, shift) instead of
   BLITZ_LIBFIN:
     * toke names the object type ("Bank", "Shape", ...).  Under that name
       the compiler options show the library's maximum; the program's
       .xtra file stores it as a pair "library number, maximum"
       (blitz2.s:15412 savextra, programs/blitz2/src/data.c savextra_c;
       read back by the same file's loader).  defmax is the maximum
       until the user changes it.
     * shift is log2 of the object size (banklib: 4 = 16 bytes).
     * The compiler reserves an a5 offset for the library (LIBMAX_VAROFF,
       programs/blitz2/src/structs.h): at offset+0 the pointer to the
       object block, at offset+4 the pointer to the used object.  For
       every object library a program uses, it records "offset, maximum <<
       shift" (blitz2.s:2750 calcmaxs, compile.c calcmaxs), and the
       program's startup allocates each block through memlib.
   What the compiler generates for BL_IA0|BL_PD0 (blitz2.s:11647 toreg,
   libcall.c toreg):
       move.l  offset(a5),a0       ; B: the object block
       cmp.w   #max,d0             ; only with runtime error checking:
       bcs     ok                  ;   number >= maximum is a
       jsr     maxerr              ;   runtime error
     ok:
       lsl.w   #shift,d0           ; number << shift
       adda.w  d0,a0               ; a0 = the object
   so the routine sees d0 = number << shift, not the number.  BL_BA*
   loads only the first line, BL_UA* loads offset+4(a5) (with checking: a
   runtime error if it is 0), BL_MD* loads `move.w #max,dn`. */
#define BL_LD(n)    ((n) << 8)              /* #ld0..#ld7 */
#define BL_LA(n)    (0x1000 | ((n) << 8))   /* #la0..#la3, #la6 */
#define BL_LPUSH    0xff00                  /* #lpush: onto the stack */
#define BL_BASE     1                       /* #base: item 0 of the objects */
#define BL_USED     2                       /* #used: current object */
#define BL_MAX      3                       /* #max: maximum setting */
#define BL_USEDOFF  4                       /* a5 offset of the used object */
#define BL_ITEM     0x80                    /* #item: object from param dN */
#define BL_PD(n)    (n)                     /* #pd0..#pd7 */
#define BL_IFBLITZ  0x8000                  /* only in Blitz mode */
#define BL_IFAMIGA  0x4000                  /* only in Amiga mode */

#define BL_LD0 0x0000   /* #ld0..#ld7: L = Library variable into dN */
#define BL_LD1 0x0100
#define BL_LD2 0x0200
#define BL_LD3 0x0300
#define BL_LD4 0x0400
#define BL_LD5 0x0500
#define BL_LD6 0x0600
#define BL_LD7 0x0700
#define BL_LA0 0x1000   /* #la0..#la3, #la6: L = Library variable into aN */
#define BL_LA1 0x1100
#define BL_LA2 0x1200
#define BL_LA3 0x1300
#define BL_LA6 0x1600

#define BL_UD0 (BL_LD0 | BL_USED)   /* #ud*, #ua*: U = Used object (LIBFIN7) */
#define BL_UD1 (BL_LD1 | BL_USED)
#define BL_UD2 (BL_LD2 | BL_USED)
#define BL_UD3 (BL_LD3 | BL_USED)
#define BL_UD4 (BL_LD4 | BL_USED)
#define BL_UD5 (BL_LD5 | BL_USED)
#define BL_UD6 (BL_LD6 | BL_USED)
#define BL_UD7 (BL_LD7 | BL_USED)
#define BL_UA0 (BL_LA0 | BL_USED)
#define BL_UA1 (BL_LA1 | BL_USED)
#define BL_UA2 (BL_LA2 | BL_USED)
#define BL_UA3 (BL_LA3 | BL_USED)
#define BL_UA6 (BL_LA6 | BL_USED)

#define BL_BD0 (BL_LD0 | BL_BASE)   /* #bd*, #ba*: B = Base, object 0 (LIBFIN7) */
#define BL_BD1 (BL_LD1 | BL_BASE)
#define BL_BD2 (BL_LD2 | BL_BASE)
#define BL_BD3 (BL_LD3 | BL_BASE)
#define BL_BD4 (BL_LD4 | BL_BASE)
#define BL_BD5 (BL_LD5 | BL_BASE)
#define BL_BD6 (BL_LD6 | BL_BASE)
#define BL_BD7 (BL_LD7 | BL_BASE)
#define BL_BA0 (BL_LA0 | BL_BASE)
#define BL_BA1 (BL_LA1 | BL_BASE)
#define BL_BA2 (BL_LA2 | BL_BASE)
#define BL_BA3 (BL_LA3 | BL_BASE)
#define BL_BA6 (BL_LA6 | BL_BASE)

#define BL_PD0 0        /* #pd0..#pd7: PD = Parameter in dN, the number of I */
#define BL_PD1 1
#define BL_PD2 2
#define BL_PD3 3
#define BL_PD4 4
#define BL_PD5 5
#define BL_PD6 6
#define BL_PD7 7
#define BL_IA0 (BL_LA0 | BL_ITEM)   /* #ia*: I = Item, object number dN (| BL_PDn) */
#define BL_IA1 (BL_LA1 | BL_ITEM)
#define BL_IA2 (BL_LA2 | BL_ITEM)
#define BL_IA3 (BL_LA3 | BL_ITEM)
#define BL_IA6 (BL_LA6 | BL_ITEM)

#define BL_MD0 (BL_LD0 | BL_MAX)    /* #md*, #ma*: M = Max setting (LIBFIN7) */
#define BL_MD1 (BL_LD1 | BL_MAX)
#define BL_MD2 (BL_LD2 | BL_MAX)
#define BL_MD3 (BL_LD3 | BL_MAX)
#define BL_MD4 (BL_LD4 | BL_MAX)
#define BL_MD5 (BL_LD5 | BL_MAX)
#define BL_MD6 (BL_LD6 | BL_MAX)
#define BL_MD7 (BL_LD7 | BL_MAX)
#define BL_MA0 (BL_LA0 | BL_MAX)
#define BL_MA1 (BL_LA1 | BL_MAX)
#define BL_MA2 (BL_LA2 | BL_MAX)
#define BL_MA3 (BL_LA3 | BL_MAX)
#define BL_MA6 (BL_LA6 | BL_MAX)

/* ------------------------------------------------------ fixed registers */

/* a5 = base of the Blitz program's globals (init results of all libs are
   globals at the header's $0A offset), a4 = locals of the current
   procedure, a3 = string work pointer (string results are written at a3,
   which then points to the terminating 0).  Declared as GCC global
   register variables; every unit that includes this header must be
   compiled with -ffixed-a3 -ffixed-a4 -ffixed-a5. */
#ifndef BL_CODE_MODEL
register UBYTE *blitz_globals __asm("a5");
register UBYTE *blitz_locals __asm("a4");
register UBYTE *blitz_string __asm("a3");
#else
/* clang (clangd in the IDE) has no global register variables */
extern UBYTE *blitz_globals, *blitz_locals, *blitz_string;
#endif

/* register parameter: LONG a BL_REG(d0) = GCC's `LONG a __asm("d0")`.
   clang does not know register parameters (amigaos GCC extension), so
   for clangd the macro is empty. */
#ifndef BL_CODE_MODEL
#define BL_REG(r) __asm(#r)
#else
#define BL_REG(r)
#endif

/* --------------------------------------------------------- helper macros */

#define BL_CAT_(a, b) a##b
#define BL_CAT(a, b) BL_CAT_(a, b)
#define BL_ID(p) BL_CAT(p, __COUNTER__)

#define BL_NARGS(...) BL_NARGS_(__VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, \
    12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define BL_NARGS_(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, \
    a15, a16, a17, a18, a19, a20, n, ...) n

/* one object of the declaration table: packed (no padding inside),
   word aligned and of even size (= `Even` after the libmacs macro) */
#define BL_HEAD __attribute__((section(".blitzlib.head"), used, aligned(2)))
#define BL_PACKED __attribute__((packed, aligned(2)))

/* code address as `Dc.l label`: 0 or a relocated address */
typedef void *bl_addr;
#define BL_A(x) ((bl_addr)(x))

/* debug-only error check routines: placed after `error`, cut away by the
   compiler when the debugger is off. */
#define BLITZ_ERRCHK __attribute__((section(".blitzlib.errchk"), used))
/* start of the error check area, for the `error` header field */
extern char blitz_errchk_start[];
#define BLITZ_ERROR_AREA blitz_errchk_start

/* a library routine: keep it even if nothing in C references it */
#define BLITZ_ROUTINE __attribute__((used))

/* ------------------------------------------------------------- header */

/* !syslibheader{libnum,firsttoke,lasttoke+1,init,return,finit,error,flags}
   MOVEQ #0,d0:RTS:Dc libnum:Dc.l 0:Dc 0,flags,first,last:Dc.l 0,init
   Dc return:Dc.l finit,0,0,0,error */
#define BLITZ_SYSLIBHEADER(libnum, first, last, init, ret, finit, error, flags) \
    static const struct BL_PACKED { UWORD moveq, rts; UWORD num; ULONG z6;      \
        UWORD za, fl, ft, lt; ULONG z12; bl_addr in; WORD rt; bl_addr fi;      \
        ULONG z20, z24, z28; bl_addr er; } BL_ID(bl_header_) BL_HEAD = {        \
        0x7000, 0x4e75, (UWORD)(libnum), 0, 0, (flags), (first), (last), 0,    \
        BL_A(init), (ret), BL_A(finit), 0, 0, 0, BL_A(error) }

/* !syslibheader with 7 parameters (no flags):
   ... Dc libnum:Dc.l 0,0:Dc first,last ... */
#define BLITZ_SYSLIBHEADER7(libnum, first, last, init, ret, finit, error) \
    BLITZ_SYSLIBHEADER(libnum, first, last, init, ret, finit, error, 0)

/* !libheader{libnum,init,return,finit,error}
   MOVEQ #0,d0:RTS:Dc libnum:Dcb.l 4,0:Dc.l init:Dc return:Dc.l finit
   Dcb.l 3,0:Dc.l error */
#define BLITZ_LIBHEADER(libnum, init, ret, finit, error) \
    BLITZ_SYSLIBHEADER(libnum, 0, 0, init, ret, finit, error, 0)

/* ------------------------------------------------------- token entries */

#define BL_KIND(v) static const UWORD BL_ID(bl_kind_)[3] BL_HEAD = { (v), 0, 0 }

#define BLITZ_ASTATEMENT()  BL_KIND(1)
#define BLITZ_USTATEMENT()  BL_KIND(8 + 1)
#define BLITZ_AFUNCTION(t)  BL_KIND(((t) << 8) + 2)
#define BLITZ_UFUNCTION()   BL_KIND(8 + 2)
#define BLITZ_ACOMMAND(t)   BL_KIND(((t) << 8) + 3)

/* !args{...}: Dc count:Dc.b types:Even */
#define BLITZ_ARGS(...) \
    static const struct BL_PACKED { UWORD n; UBYTE t[BL_NARGS(__VA_ARGS__)]; } \
        BL_ID(bl_args_) BL_HEAD = { BL_NARGS(__VA_ARGS__), { __VA_ARGS__ } }
#define BLITZ_ARGS0() static const UWORD BL_ID(bl_args_) BL_HEAD = 0

/* !repargs{first,num,...}: Dc first<<12|num<<8|count:Dc.b types:Even */
#define BLITZ_REPARGS(first, num, ...) \
    static const struct BL_PACKED { UWORD n; UBYTE t[BL_NARGS(__VA_ARGS__)]; } \
        BL_ID(bl_repargs_) BL_HEAD = {                                          \
        ((first) << 12) | ((num) << 8) | BL_NARGS(__VA_ARGS__), { __VA_ARGS__ } }

/* !libs{lib,reg,...}: Dc lib,reg,...:Dc 0 */
#define BLITZ_LIBS(...) \
    static const UWORD BL_ID(bl_libs_)[] BL_HEAD = { __VA_ARGS__, 0 }
#define BLITZ_LIBS0() static const UWORD BL_ID(bl_libs_) BL_HEAD = 0

/* !subs{code,error1,error2}: Dc.l error1,code,error2 */
#define BLITZ_SUBS(code, err1, err2) \
    static const struct BL_PACKED { bl_addr e1, c, e2; } \
        BL_ID(bl_subs_) BL_HEAD = { BL_A(err1), BL_A(code), BL_A(err2) }

/* !name{"name","help"}: Dc -1:Dc.l 0:Dc 0:Dc.b name,0:Dc.b help,0:Even */
#define BLITZ_NAME(name, help) \
    static const struct BL_PACKED { UWORD m1; ULONG next; UWORD num;      \
        char n[sizeof(name)]; char h[sizeof(help)]; } \
        BL_ID(bl_name_) BL_HEAD = { 0xffff, 0, 0, name, help }

/* label:!name{"name","help"} - name entry whose name string is referenced
   elsewhere (e.g. as !libfin's toke of an object library): &label.n */
#define BLITZ_NAME_AT(label, name, help) \
    static const struct BL_PACKED { UWORD m1; ULONG next; UWORD num;      \
        char n[sizeof(name)]; char h[sizeof(help)]; } \
        label BL_HEAD = { 0xffff, 0, 0, name, help }

/* !nullsub{routine,error1,error2,lib,reg,...}:
   Dc 0,0,0:!libs{...}:Dc.l error1,routine,error2 */
#define BLITZ_NULLSUB(code, err1, ...) \
    BL_KIND(0); BL_NULLSUB_LIBS(__VA_ARGS__); BLITZ_SUBS(code, err1, BL_FIRST(__VA_ARGS__))
#define BL_FIRST(a, ...) a
#define BL_NULLSUB_LIBS(err2, ...) \
    static const UWORD BL_ID(bl_libs_)[] BL_HEAD = { __VA_ARGS__ __VA_OPT__(,) 0 }

/* !dumtoke{"name","help",label}: Dc 8,0,0:Dc.l 0:Dc 0:label:Dc.b name,0
   Dc.b help,0:Even.  `label` (for BLITZ_LIBFIN7's toke) is &label.n */
#define BLITZ_DUMTOKE(label, name, help) \
    static const struct BL_PACKED { UWORD k[3]; ULONG next; UWORD num;    \
        char n[sizeof(name)]; char h[sizeof(help)]; } label BL_HEAD = {      \
        { 8, 0, 0 }, 0, 0, name, help }

/* !libfin: Dc -1:Dc.l 0 */
#define BLITZ_LIBFIN() \
    static const struct BL_PACKED { UWORD m1; ULONG z; } \
        BL_ID(bl_libfin_) BL_HEAD = { 0xffff, 0 }

/* !libfin{toke,load,save,use,free,defmax,shift} (object libraries):
   Dc -1:Dc.l toke,load,save,use,free:Dc 0,defmax,shift
   toke: the object type's name (a BLITZ_NAME_AT / BLITZ_DUMTOKE label
   .n), shown with its maximum in the compiler options and stored in the
   .xtra file; load, save, use, free: the library's routines for the
   object type that the compiler calls itself (blitz2.s libfin); defmax: the default maximum; shift:
   log2 of the object size.  See "Object libraries" at the register codes
   above */
#define BLITZ_LIBFIN7(toke, load, save, use, free, defmax, shift) \
    static const struct BL_PACKED { UWORD m1; bl_addr t, l, s, u, f;    \
        UWORD z, mx, sh; } BL_ID(bl_libfin_) BL_HEAD = { 0xffff,           \
        BL_A(toke), BL_A(load), BL_A(save), BL_A(use), BL_A(free), 0,      \
        (defmax), (shift) }

/* --------------------------------------- AmigaOS call libraries (deflibs) */

/* The libraries of blitz2:deflibs declare AmigaOS library calls (`Open_`,
   `AllocMem_` ...): the compiler loads the arguments into the registers
   and calls the LVO with a6 = the base that the init nullsub of the
   library `oslib` returned (blitz2.s stamigalib).  The originals
   (cd/extras/blitzlibs/amigalibs) were not built with libmacs; their
   header is the one of !libheader, but starts with 4 zero bytes instead
   of MOVEQ #0,d0:RTS:
     Dc.l 0:Dc libnum:Dcb.l 4,0:Dc.l init:Dc return:Dc.l finit
     Dcb.l 3,0:Dc.l 0                                     (error = 0) */
#define BLITZ_OSLIBHEADER(libnum, init, ret, finit) \
    static const struct BL_PACKED { UWORD z0, z2; UWORD num; ULONG z6;     \
        UWORD za, fl, ft, lt; ULONG z12; bl_addr in; WORD rt; bl_addr fi;  \
        ULONG z20, z24, z28; bl_addr er; } BL_ID(bl_header_) BL_HEAD = {    \
        0, 0, (UWORD)(libnum), 0, 0, 0, 0, 0, 0,                            \
        BL_A(init), (ret), BL_A(finit), 0, 0, 0, 0 }

/* register codes of BLITZ_OSCALL (stamigalib: bit 4 = address register,
   bits 0-2 = register number) */
#define BL_OSR_d0 0x00
#define BL_OSR_d1 0x01
#define BL_OSR_d2 0x02
#define BL_OSR_d3 0x03
#define BL_OSR_d4 0x04
#define BL_OSR_d5 0x05
#define BL_OSR_d6 0x06
#define BL_OSR_d7 0x07
#define BL_OSR_a0 0x10
#define BL_OSR_a1 0x11
#define BL_OSR_a2 0x12
#define BL_OSR_a3 0x13
#define BL_OSR_a4 0x14
#define BL_OSR_a5 0x15
#define BL_OSR_a6 0x16
#define BL_OSR_a7 0x17

/* BL_OSREGS(d1, a0) -> BL_OSR_d1, BL_OSR_a0,   (up to 19 registers) */
#define BL_OSREGS(...) BL_CAT(BL_OSR_N, BL_NARGS(__VA_ARGS__))(__VA_ARGS__)
#define BL_OSR_N1(r) BL_OSR_##r,
#define BL_OSR_N2(r, ...) BL_OSR_##r, BL_OSR_N1(__VA_ARGS__)
#define BL_OSR_N3(r, ...) BL_OSR_##r, BL_OSR_N2(__VA_ARGS__)
#define BL_OSR_N4(r, ...) BL_OSR_##r, BL_OSR_N3(__VA_ARGS__)
#define BL_OSR_N5(r, ...) BL_OSR_##r, BL_OSR_N4(__VA_ARGS__)
#define BL_OSR_N6(r, ...) BL_OSR_##r, BL_OSR_N5(__VA_ARGS__)
#define BL_OSR_N7(r, ...) BL_OSR_##r, BL_OSR_N6(__VA_ARGS__)
#define BL_OSR_N8(r, ...) BL_OSR_##r, BL_OSR_N7(__VA_ARGS__)
#define BL_OSR_N9(r, ...) BL_OSR_##r, BL_OSR_N8(__VA_ARGS__)
#define BL_OSR_N10(r, ...) BL_OSR_##r, BL_OSR_N9(__VA_ARGS__)
#define BL_OSR_N11(r, ...) BL_OSR_##r, BL_OSR_N10(__VA_ARGS__)
#define BL_OSR_N12(r, ...) BL_OSR_##r, BL_OSR_N11(__VA_ARGS__)
#define BL_OSR_N13(r, ...) BL_OSR_##r, BL_OSR_N12(__VA_ARGS__)
#define BL_OSR_N14(r, ...) BL_OSR_##r, BL_OSR_N13(__VA_ARGS__)
#define BL_OSR_N15(r, ...) BL_OSR_##r, BL_OSR_N14(__VA_ARGS__)
#define BL_OSR_N16(r, ...) BL_OSR_##r, BL_OSR_N15(__VA_ARGS__)
#define BL_OSR_N17(r, ...) BL_OSR_##r, BL_OSR_N16(__VA_ARGS__)
#define BL_OSR_N18(r, ...) BL_OSR_##r, BL_OSR_N17(__VA_ARGS__)
#define BL_OSR_N19(r, ...) BL_OSR_##r, BL_OSR_N18(__VA_ARGS__)
/* number of registers, 0 for none */
#define BL_NOSREGS(...) (0 __VA_OPT__(+ BL_NARGS(__VA_ARGS__)))

/* BLITZ_OSCALL(oslib, lvo, "Name_", "(help)", regs...) - one AmigaOS call
   (token kind 6): `oslib` = number of the library whose init result is
   the base (a6), `lvo` = negative offset, regs = the registers of the
   arguments in order (d0..d7, a0..a6), none for a call without arguments:
     Dc 6,0,0:Dc oslib,lvo:Dc.b regs...,$ff:Even
     Dc.l 0:Dc 0:Dc.b name,0:Dc.b help,0:Even */
#define BLITZ_OSCALL(oslib, lvo, name, help, ...) \
    static const struct BL_PACKED { UWORD k[3]; UWORD lib; WORD off;        \
        UBYTE r[(BL_NOSREGS(__VA_ARGS__) + 2) & ~1]; ULONG next; UWORD num; \
        char n[sizeof(name)]; char h[sizeof(help)]; }                       \
        BL_ID(bl_oscall_) BL_HEAD = { { 6, 0, 0 }, (oslib), (lvo),          \
        { __VA_OPT__(BL_OSREGS(__VA_ARGS__)) 0xff }, 0, 0, name, help }

/* ------------------------------------------------- labelled table entries */

/* `label:!nullsub{...}` - a nullsub whose entry address is used in the
   header (init/finit point to the ENTRY in the table, not to the code).
   Declare the label before the header with BLITZ_ENTRY_DECL(label). */
#define BLITZ_ENTRY_DECL(label) extern const UWORD label[]

/* BLITZ_ENTRY_C(f) - a code label (routine, thunk, BLITZ_AB_ROUTINE ...)
   as a char array `e_f`, for table entries that need address arithmetic
   or an address that C cannot take from a function declaration in a
   constant initialiser. */
#define BLITZ_ENTRY_C(f) extern char e_##f[] __asm__("_" #f)

/* Code / error check address + 1 in !subs / !nullsub (blitz2.s):
     BLITZ_AMIGAONLY(code)      code + 1: the routine exists in Amiga mode
                                only (a Blitz mode call is a compile error)
     BLITZ_BLITZONLY            err1 = 1: Blitz mode only, no check
     BLITZ_BLITZONLY_CHK(chk)   err1 = chk + 1: Blitz mode only, with the
                                error check `chk` */
#define BLITZ_AMIGAONLY(code)    ((char *)(code) + 1)
#define BLITZ_BLITZONLY          ((void *)1)
#define BLITZ_BLITZONLY_CHK(chk) ((char *)(chk) + 1)
#define BLITZ_NULLSUB_AT(label, code, err1, ...) \
    const UWORD label[3] BL_HEAD = { 0, 0, 0 }; \
    BL_NULLSUB_LIBS(__VA_ARGS__); BLITZ_SUBS(code, err1, BL_FIRST(__VA_ARGS__))

/* --------------------------------------------- calling other libraries */

/* Address of a library routine for ALibJsr/BLibJsr: the original compiler
   writes `JSR <long>` with the placeholder ($8000|form)<<16 | token
   (BLibJsr: $C000|form, Blitz mode) and a relocation; the compiler that
   links the library replaces it by the real address (blitz2.s handlejsrs).
   blitz_tok is a linker symbol at offset 0 of the library, so
   blitz_tok+placeholder is exactly that relocated long. */
extern char blitz_tok[];
#define BL_TOKVAL(tok, form) (0x80000000UL | ((ULONG)(form) << 16) | (UWORD)(tok))
#define BL_BTOKVAL(tok, form) (0xC0000000UL | ((ULONG)(form) << 16) | (UWORD)(tok))
#define BLITZ_TOKE(tok)      ((void *)(blitz_tok + BL_TOKVAL(tok, 0)))
/* the same placeholder as text for inline assembler, from the library
   number and the low part of the token (libnum<<7 + n):
   "jsr _blitz_tok+" BL_ASMTOK(LIB_printlib, 1) */
#define BL_STR_(x) #x
#define BL_STR(x) BL_STR_(x)
#define BL_ASMTOK(lib, n) "0x80000000+((" BL_STR(lib) "<<7)+" BL_STR(n) ")"

/* Registers passed to / returned from a routine of another library. */
struct bl_regs {
    ULONG d0, d1, a0, a1;
};

/* ALibJsr tok[,form] - call a routine of another (possibly original
   assembler) library.  In: r.d0/d1/a0/a1, out: the same registers.  The
   called code may change every register except a4, a5, a7 (Blitz
   convention), so all others are declared clobbered.  a3 (blitz_string)
   is an in/out operand of every ALibJsr/BLibJsr macro: a string function
   (UStr$, Str$ ...) writes its result at a3 and leaves a3 behind it, and
   any routine of another library may be one.  Without the operand GCC
   would take a3 as unchanged by the call and could use a value it knows
   from before (e.g. a3 == the buffer it just stored there).  a3 is a
   fixed register (-ffixed-a3), so the operand stays in a3 and GCC never
   saves or restores it: a routine that moves a3 for its caller still
   does. */
#define BL_A3_ "+a"(blitz_string)
#define BLITZ_ALIBJSR(tok, r) BL_JSR_(BL_TOKVAL(tok, 0), r)
#define BLITZ_ALIBJSR_FORM(tok, form, r) BL_JSR_(BL_TOKVAL(tok, form), r)
#define BLITZ_BLIBJSR(tok, r) BL_JSR_(BL_BTOKVAL(tok, 0), r)
#ifndef BL_CODE_MODEL
#define BL_JSR_(val, r) ({                                                  \
    register ULONG _d0 __asm("d0") = (r).d0;                                \
    register ULONG _d1 __asm("d1") = (r).d1;                                \
    register ULONG _a0 __asm("a0") = (r).a0;                                \
    register ULONG _a1 __asm("a1") = (r).a1;                                \
    __asm volatile("jsr _blitz_tok+%c5"                                     \
        : "+d"(_d0), "+d"(_d1), "+a"(_a0), "+a"(_a1), BL_A3_ : "i"(val)     \
        : "d2", "d3", "d4", "d5", "d6", "d7", "a2", "a6", "cc", "memory");  \
    (r).d0 = _d0; (r).d1 = _d1; (r).a0 = _a0; (r).a1 = _a1; _d0; })
#else
ULONG bl_jsr_clang(ULONG val, struct bl_regs *r);
#define BL_JSR_(val, r) bl_jsr_clang(val, &(r))
#endif

/* BLITZ_BASEJSR(base, off, r) - `JSR off(a6)` with a6 = base: call
   through a vector or a library base whose target has an open contract,
   e.g. fontlib's previous input handler (any library's or program's
   handler) or a routine of a third-party library the port does not rely
   on.  In/out: r.d0/d1/a0/a1 as with BLITZ_ALIBJSR; the callee may change
   every register except a3-a5/a7 (a6 included), so all others are
   declared clobbered.  Unlike BLITZ_ALIBJSR, a3 is no operand: the
   routines called this way keep a3.  A library by the AmigaOS convention
   and memlib's membase table take BLITZ_SYSJSR (below). */
#ifndef BL_CODE_MODEL
#define BLITZ_BASEJSR(base, off, r) ({                                      \
    register ULONG _d0 __asm("d0") = (r).d0;                                \
    register ULONG _d1 __asm("d1") = (r).d1;                                \
    register ULONG _a0 __asm("a0") = (r).a0;                                \
    register ULONG _a1 __asm("a1") = (r).a1;                                \
    register const void *_a6 __asm("a6") = (base);                          \
    __asm volatile("jsr %c5(a6)"                                            \
        : "+d"(_d0), "+d"(_d1), "+a"(_a0), "+a"(_a1), "+a"(_a6) : "i"(off)  \
        : "d2", "d3", "d4", "d5", "d6", "d7", "a2", "cc", "memory");        \
    (r).d0 = _d0; (r).d1 = _d1; (r).a0 = _a0; (r).a1 = _a1; _d0; })
#else
ULONG bl_basejsr_clang(const void *base, int off, struct bl_regs *r);
#define BLITZ_BASEJSR(base, off, r) bl_basejsr_clang(base, off, &(r))
#endif

/* BLITZ_SYSJSR(base, off, r), BLITZ_SYSJSR1(base, off, x0),
   BLITZ_SYSJSR2(base, off, x0, x1) - `JSR off(a6)` with a6 = base to a
   routine that keeps d2-d7 and a2-a6:
   - a library by the AmigaOS convention (d0/d1/a0/a1 scratch, the rest
     kept): exec, graphics, intuition, diskfont, mathffp, rexxsyslib,
     translator, ...;
   - memlib's membase table (blitzhelp.h MEMBASE_xxx: JSR (a6) localloc,
     JSR 4(a6) locfree, +8 the global versions), by a tested Blitz
     contract: the KEEPS cases of libs/tests/systest.c ("memlib localloc
     (table)" and its siblings, recorded from the original) state that
     localloc and globalloc change d0/d1 only, locfree and globfree d0/a1
     only.
   Only d0, d1, a0 and a1 are declared changed, so GCC keeps its values in
   the other registers across the call instead of saving d2-d7/a2/a6
   around it; a6 is an input and stays.  `cc` and `memory` are clobbered
   (the callee writes memory the caller reads: AllocMem, CloseFont, ...).
   - BLITZ_SYSJSR: r.d0/d1/a0/a1 in and out through struct bl_regs, the
     general form (also for a second result, e.g. a0 of rexxsyslib
     ErrorMsg); result d0.
   - BLITZ_SYSJSR1: d0 = x0 in, result d0; d1/a0/a1 changed.
   - BLITZ_SYSJSR2: d0 = x0 and d1 = x1 in, result d0; a0/a1 changed.
     The usual form for mathffp (a6 = the ffplib base, !libs
     #ffplib,#la6): BLITZ_SYSJSR2(ffp, LVO_SPAdd, a, b).
   A routine that may change more (a handler, "whatever a6 holds") takes
   BLITZ_BASEJSR; a call with an argument in d2-d7 or a2-a5 goes through
   the NDK inline (proto headers with the shadow headers of ndk/safe/). */
#ifndef BL_CODE_MODEL
#define BLITZ_SYSJSR(base, off, r) ({                                       \
    register ULONG _d0 __asm("d0") = (r).d0;                                \
    register ULONG _d1 __asm("d1") = (r).d1;                                \
    register ULONG _a0 __asm("a0") = (r).a0;                                \
    register ULONG _a1 __asm("a1") = (r).a1;                                \
    register const void *_a6 __asm("a6") = (base);                          \
    __asm volatile("jsr %c5(a6)"                                            \
        : "+d"(_d0), "+d"(_d1), "+a"(_a0), "+a"(_a1) : "a"(_a6), "i"(off)   \
        : "cc", "memory");                                                  \
    (r).d0 = _d0; (r).d1 = _d1; (r).a0 = _a0; (r).a1 = _a1; _d0; })
#define BLITZ_SYSJSR1(base, off, x0) ({                                     \
    register ULONG _d0 __asm("d0") = (x0);                                  \
    register const void *_a6 __asm("a6") = (base);                          \
    __asm volatile("jsr %c2(a6)"                                            \
        : "+d"(_d0) : "a"(_a6), "i"(off)                                    \
        : "d1", "a0", "a1", "cc", "memory");                                \
    _d0; })
#define BLITZ_SYSJSR2(base, off, x0, x1) ({                                 \
    register ULONG _d0 __asm("d0") = (x0);                                  \
    register ULONG _d1 __asm("d1") = (x1);                                  \
    register const void *_a6 __asm("a6") = (base);                          \
    __asm volatile("jsr %c3(a6)"                                            \
        : "+d"(_d0), "+d"(_d1) : "a"(_a6), "i"(off)                         \
        : "a0", "a1", "cc", "memory");                                      \
    _d0; })
#else
#define BLITZ_SYSJSR(base, off, r) bl_basejsr_clang(base, off, &(r))
#define BLITZ_SYSJSR1(base, off, x0) ({                                     \
    struct bl_regs r_ = { .d0 = (x0) }; BLITZ_SYSJSR(base, off, r_); })
#define BLITZ_SYSJSR2(base, off, x0, x1) ({                                 \
    struct bl_regs r_ = { .d0 = (x0), .d1 = (x1) };                         \
    BLITZ_SYSJSR(base, off, r_); })
#endif

/* BLITZ_ALIBJSR_A6(tok, base, x) - ALibJsr tok with a6 = `base`, in d0 =
   x, result d0.  BLITZ_ALIBJSR does not load a6 (and declares it
   clobbered), but some routines need a library base there, e.g.
   floatquicklib $D300 qutofl the ffplib base: the original calls it from
   a routine with !libs #ffplib,#la6 and a6 still loaded.  a6 is declared
   changed; a2 (the compiler's argument stack) is kept like by every
   library routine (see notes/cmdlibs-port.md), a4/a5/a7 anyway, a3 is
   in/out (see BLITZ_ALIBJSR).  (from
   vallib Int) */
#ifndef BL_CODE_MODEL
#define BLITZ_ALIBJSR_A6(tok, base, x) ({                                   \
    register ULONG _d0 __asm("d0") = (x);                                   \
    register const void *_a6 __asm("a6") = (base);                          \
    __asm volatile("jsr _blitz_tok+%c3"                                     \
        : "+d"(_d0), "+a"(_a6), BL_A3_ : "i"(BL_TOKVAL(tok, 0))             \
        : "d1", "d2", "d3", "d4", "d5", "d6", "d7", "a0", "a1", "cc", "memory"); \
    _d0; })
#else
#define BLITZ_ALIBJSR_A6(tok, base, x) ((void)(base), (ULONG)(x))
#endif

/* ALibJsr with d0 = x, a2 = a2v (argument stack for the callee, e.g. a
   string length or type word) and a6 = a6v; result d0.  The callee may
   change every register except a4/a5/a7; a3 is in/out (see
   BLITZ_ALIBJSR). */
#ifndef BL_CODE_MODEL
#define BLITZ_ALIBJSR_A2A6(tok, x, a2v, a6v) ({                             \
    register ULONG _d0 __asm("d0") = (ULONG)(x);                            \
    register const void *_a2 __asm("a2") = (a2v);                           \
    register const void *_a6 __asm("a6") = (a6v);                           \
    __asm volatile("jsr _blitz_tok+%c4"                                     \
        : "+d"(_d0), "+a"(_a2), "+a"(_a6), BL_A3_ : "i"(BL_TOKVAL(tok, 0))  \
        : "d1", "d2", "d3", "d4", "d5", "d6", "d7", "a0", "a1", "cc", "memory"); \
    _d0; })
#else
#define BLITZ_ALIBJSR_A2A6(tok, x, a2v, a6v) ((void)(a2v), (void)(a6v), (ULONG)(x))
#endif

/* BLITZ_ALIBJSR_CCR(tok, r, "cc") - BLITZ_ALIBJSR for a routine that
   answers in the condition codes (iffiolib: Z = ok): in/out r.d0/d1/a0/a1
   like BLITZ_ALIBJSR, the result is `Scc` right after the call (0 or
   0xff), e.g. BLITZ_ALIBJSR_CCR(TOK_iffopen, r, "ne") = 0xff on error.
   (from audiolib/ilbmifflib IFFCALL/IFFJSR) */
#ifndef BL_CODE_MODEL
#define BLITZ_ALIBJSR_CCR(tok, r, cc) ({                                    \
    register ULONG _d0 __asm("d0") = (r).d0;                                \
    register ULONG _d1 __asm("d1") = (r).d1;                                \
    register ULONG _a0 __asm("a0") = (r).a0;                                \
    register ULONG _a1 __asm("a1") = (r).a1;                                \
    UBYTE _cc;                                                              \
    __asm volatile("jsr _blitz_tok+%c6\n\ts" cc " %4"                       \
        : "+d"(_d0), "+d"(_d1), "+a"(_a0), "+a"(_a1), "=m"(_cc), BL_A3_     \
        : "i"(BL_TOKVAL(tok, 0))                                            \
        : "d2", "d3", "d4", "d5", "d6", "d7", "a2", "a6", "cc", "memory");  \
    (r).d0 = _d0; (r).d1 = _d1; (r).a0 = _a0; (r).a1 = _a1; _cc; })
#else
#define BLITZ_ALIBJSR_CCR(tok, r, cc) ((void)(r), (UBYTE)0)
#endif

/* BLITZ_ALIBJSR_CCR_D0A0(tok, "cc", d0p, a0v) - the short form: in d0 =
   *d0p, a0 = a0v (d1/a1 undefined), out *d0p = d0; result `Scc` after
   the call.  Same register contract as BLITZ_ALIBJSR.  (from ilbmifflib
   IFFJSR: BLITZ_ALIBJSR_CCR_D0A0(TOK_iffbyte, "eq", &d0, st) = ok) */
#ifndef BL_CODE_MODEL
#define BLITZ_ALIBJSR_CCR_D0A0(tok, cc, d0p, a0v) ({                        \
    register ULONG _d0 __asm("d0") = *(d0p);                                \
    register ULONG _d1 __asm("d1");                                         \
    register const void *_a0 __asm("a0") = (a0v);                           \
    register ULONG _a1 __asm("a1");                                         \
    UBYTE _cc;                                                              \
    __asm volatile("jsr _blitz_tok+%c6\n\ts" cc " %4"                       \
        : "+d"(_d0), "=d"(_d1), "+a"(_a0), "=a"(_a1), "=m"(_cc), BL_A3_     \
        : "i"(BL_TOKVAL(tok, 0))                                            \
        : "d2", "d3", "d4", "d5", "d6", "d7", "a2", "a6", "cc", "memory");  \
    *(d0p) = _d0; _cc; })
#else
#define BLITZ_ALIBJSR_CCR_D0A0(tok, cc, d0p, a0v) ((void)(a0v), (UBYTE)*(d0p))
#endif

/* BLITZ_ALIBJSR_D4(tok, d0, d1, d2, d3) / BLITZ_BLIBJSR_D4 - a routine of
   another library with its arguments in d0-d3 (BLITZ_ALIBJSR only passes
   d0/d1/a0/a1), e.g. spriteslib ShowSprite (sprite, x, y, channel).
   Result d0; the callee may change every register except a4/a5/a7; a3
   is in/out (see BLITZ_ALIBJSR). */
#ifndef BL_CODE_MODEL
#define BL_JSR_D4_(val, x0, x1, x2, x3) ({                                  \
    register ULONG _d0 __asm("d0") = (x0);                                  \
    register ULONG _d1 __asm("d1") = (x1);                                  \
    register ULONG _d2 __asm("d2") = (x2);                                  \
    register ULONG _d3 __asm("d3") = (x3);                                  \
    __asm volatile("jsr _blitz_tok+%c5"                                     \
        : "+d"(_d0), "+d"(_d1), "+d"(_d2), "+d"(_d3), BL_A3_ : "i"(val)     \
        : "d4", "d5", "d6", "d7", "a0", "a1", "a2", "a6", "cc", "memory");  \
    _d0; })
#else
#define BL_JSR_D4_(val, x0, x1, x2, x3) \
    ((void)(x1), (void)(x2), (void)(x3), (ULONG)(x0))
#endif
#define BLITZ_ALIBJSR_D4(tok, x0, x1, x2, x3) \
    BL_JSR_D4_(BL_TOKVAL(tok, 0), x0, x1, x2, x3)
#define BLITZ_BLIBJSR_D4(tok, x0, x1, x2, x3) \
    BL_JSR_D4_(BL_BTOKVAL(tok, 0), x0, x1, x2, x3)

/* ------------------------------------------------------------- thunks */

/* BL_ASM_FUNC_(attrs, name, text) - the function `name` (void name(void),
   with `attrs`, e.g. BLITZ_ERRCHK) whose code is only the assembler
   `text`, which ends with its own return (rts, jmp) or is data: GCC adds
   nothing but `.align 2` (= .even), `.globl` and the label; noipa: GCC
   must not fold two routines with the same text into one (identical code
   folding would make one a call of the other, which with
   __builtin_unreachable has no return behind it).  All
   assembler routines of this file are such functions and not top-level
   asm: GCC switches to their section itself and knows the section it is
   in behind them.  A section switch in top-level asm is invisible to GCC
   (the assembler of this toolchain has no .pushsection/.popsection to
   undo it), so a function behind it that GCC places in the section it
   believes current gets no section directive and lands in the section
   the asm left (#1056: check functions behind BLITZ_ERRCHK_THUNK in the
   normal code). */
#ifndef BL_CODE_MODEL
#define BL_NOIPA_ __attribute__((noipa))
#else
#define BL_NOIPA_                       /* clang has no noipa */
#endif
#define BL_ASM_FUNC_(attrs, name, text)                                     \
    attrs BL_NOIPA_ void name(void)                                         \
    {                                                                       \
        __asm volatile(text);                                               \
        __builtin_unreachable();                                            \
    }

/* Library entry with a register contract that a C function cannot express
   itself.  `entry` is the label used in the token table.  The thunk saves
   the registers `save` - the ones the original routine preserves but GCC
   may change (d0, d1, a0, a1; GCC itself keeps d2-d7/a2-a6) - calls the C
   function `cfunc` (its register parameters are passed through unchanged)
   and restores them; movem does not change the CCR.
     BLITZ_THUNK(entry, cfunc, "d0-d1/a0-a1")
     BLITZ_THUNK_Z(entry, cfunc, save)   CCR result: Z = (C result == 0);
         the C function must return a 32 bit type (tst.l d0) - not BOOL,
         which is a 16 bit type in exec/types.h
     BLITZ_THUNK_PRE(entry, cfunc, save, pre)
         `pre` = assembler run after saving, before the call, e.g. to load
         a stack argument into a register parameter.  The stack then
         holds: saved registers (4 bytes each), the caller's return
         address, the caller's stack arguments. */
#define BLITZ_THUNK(entry, cfunc, save) BL_THUNK_(entry, cfunc, save, "", "")

/* BLITZ_ALIAS(entry, cfunc) - the entry `entry` of the token table is the
   C function `cfunc` itself, without a thunk: the entry of a user-level
   statement or function, which keeps a4-a7 and (a function) a3 and
   nothing else must survive it (the register rule, docs/libs/deviations.md
   "Register preservation"); GCC keeps d2-d7/a2/a6 anyway, a3-a5 are
   fixed.  `cfunc` is a BLITZ_ROUTINE (used), so GCC emits it under its
   own name with the register parameters it declares.  An exception of
   the rule that GCC's code keeps by nature is an alias too; its KEEPS
   statement in the library tests states the registers. */
#define BLITZ_ALIAS(entry, cfunc) \
    __asm__("   .globl  _" #entry "\n   .set    _" #entry ",_" #cfunc)

/* BLITZ_ENTRYPOINT - in front of the C function of a user-level entry
   (static BLITZ_ENTRYPOINT BLITZ_ROUTINE ULONG f_c(...)) that GCC is to
   compile without saving the callee-saved registers d2-d7/a2/a6 (GCC's
   __entrypoint): the register rule does not need d2-d7/a2 kept, and the
   saves are the cost of a short routine.  BLITZ_ALIAS_EP(entry, cfunc)
   behind it makes it the entry and marks it for the build check
   (blitzlib entrypoints, tools/lib/epcheck), which refuses:
     - a6 written without a save at the routine's start (the rule keeps
       a6; __entrypoint gives it free, so a routine that loads a library
       base must not be one);
     - a call or jump into the routine from other code: GCC does not know
       at a call site that the routine keeps nothing, so it must never be
       called from C;
     - folding with a function of the same body: GCC's identical code
       folding merges them (the plain one loses its saves); no_icf keeps
       the routine apart.
   Only for entries; never for internal tokens (they keep d2-d7). */
#ifndef BL_CODE_MODEL
#define BLITZ_ENTRYPOINT __entrypoint __attribute__((no_icf))
#else
#define BLITZ_ENTRYPOINT
#endif
#define BLITZ_ALIAS_EP(entry, cfunc)                                        \
    __asm__("   .globl  _" #entry "\n   .set    _" #entry ",_" #cfunc       \
            "\n   .globl  ___blitz_entrypoint_" #entry                       \
            "\n   .set    ___blitz_entrypoint_" #entry ",_" #cfunc)
/* BLITZ_EP_MARK(cfunc) - the marker of BLITZ_ALIAS_EP alone, for an
   __entrypoint routine that is the Blitz mode code of a
   BLITZ_AB_ROUTINE (reached through its `Dc.l`, not a symbol of its own) */
#define BLITZ_EP_MARK(cfunc)                                                \
    __asm__("   .globl  ___blitz_entrypoint_" #cfunc                         \
            "\n   .set    ___blitz_entrypoint_" #cfunc ",_" #cfunc)
/* BLITZ_EP_KEEP_A6() / BLITZ_EP_KEEP_A6_END() - at the start and the end
   of an __entrypoint routine that needs more address registers than GCC
   finds in a0-a2: the caller's a6 is a live hard register from the one
   to the other, so GCC does not take a6 (the build check refuses a
   routine that writes it) and keeps its pointers in a0-a2 and d0-d7 */
#ifndef BL_CODE_MODEL
#define BLITZ_EP_KEEP_A6()                                                  \
    register ULONG bl_keep_a6_ __asm("a6");                                 \
    __asm__ volatile("" : "=a"(bl_keep_a6_))
#define BLITZ_EP_KEEP_A6_END() __asm__ volatile("" : : "a"(bl_keep_a6_))
#else
#define BLITZ_EP_KEEP_A6()
#define BLITZ_EP_KEEP_A6_END()
#endif
#define BLITZ_THUNK_Z(entry, cfunc, save) \
    BL_THUNK_(entry, cfunc, save, "", "   tst.l   d0\n")
#define BLITZ_THUNK_PRE(entry, cfunc, save, pre) \
    BL_THUNK_(entry, cfunc, save, "   " pre "\n", "")
/*   BLITZ_THUNK_POST(entry, cfunc, save, post)
         `post` = assembler run after the call, before restoring `save`,
         e.g. "move.l d0,a0" (result in a0: a0 not in `save`) or
         "move.w d0,2(sp)" (word result, the caller's upper word of d0
         kept: `save` must start with d0, its low word is at 2(sp)). */
#define BLITZ_THUNK_POST(entry, cfunc, save, post) \
    BL_THUNK_(entry, cfunc, save, "", "   " post "\n")
#define BL_THUNK_(entry, cfunc, save, pre, tail)                            \
    BL_ASM_FUNC_(, entry,                                                   \
            "   movem.l " save ",-(sp)\n" pre                               \
            "   jsr     _" #cfunc "\n" tail                                 \
            "   movem.l (sp)+," save "\n"                                   \
            "   rts\n")

/* BLITZ_THUNK_POP(entry, cfunc, save, pre, tail, pop) - like the thunks
   above for a routine that removes its own stack arguments (the original
   ends with e.g. MOVE.l (a7)+,(a7):RTS): after restoring `save`, the
   return address is moved up by `pop` bytes and the stack pointer
   advanced - with movem/lea only, so the CCR set by `tail` survives.
     save  registers to keep, may be "" (nothing saved)
     pre   assembler before the call (stack: saved registers, return
           address, arguments - as with BLITZ_THUNK_PRE), may be ""
     tail  assembler after the call, e.g. "tst.l d0" for a CCR result
     pop   number of argument bytes to remove (4, 8 ...; 0 = none, for a
           routine with a CCR result but nothing to save)
   (added by the mathstr group: strings2lib, strcomplib) */
#define BLITZ_THUNK_POP(entry, cfunc, save, pre, tail, pop)                 \
    BL_ASM_FUNC_(, entry,                                                   \
            "   .ifnc \"" save "\",\"\"\n"                                  \
            "   movem.l " save ",-(sp)\n"                                   \
            "   .endif\n"                                                   \
            "   " pre "\n"                                                  \
            "   jsr     _" #cfunc "\n"                                      \
            "   " tail "\n"                                                 \
            "   .ifnc \"" save "\",\"\"\n"                                  \
            "   movem.l (sp)+," save "\n"                                   \
            "   .endif\n"                                                   \
            "   .if " #pop "\n"                                              \
            "   movem.l a0,-(sp)\n"                                         \
            "   movem.l 4(sp),a0\n"                                         \
            "   movem.l a0,4+" #pop "(sp)\n"                                \
            "   movem.l (sp)+,a0\n"                                         \
            "   lea     " #pop "(sp),sp\n"                                  \
            "   .endif\n"                                                   \
            "   rts\n")

/* ------------------------------------------- error checks (debugger only) */

/* BLITZ_ERRCHK_THUNK(entry, cfunc, save) - BLITZ_THUNK for an error check
   routine (!subs/!nullsub err1/err2): the thunk goes into the error check
   area (.blitzlib.errchk) like the C function `cfunc`, which is declared
   BLITZ_ERRCHK.  An error check runs right before the routine, with the
   routine's arguments in the registers; it must keep them and everything
   else the original check keeps: `save` = the registers of d0-d1/a0-a1
   the original does not change (GCC keeps d2-d7/a2-a6 itself), e.g.
   "d0-d1/a0-a1" for a check that only compares (vallib intchk),
   "d0-d1/a1" when the original changes a0 (linklistlib).  The check
   function can take its inputs as register parameters (d0, a2 ...);
   they arrive unchanged. */
#define BLITZ_ERRCHK_THUNK(entry, cfunc, save)                              \
    BL_ASM_FUNC_(BLITZ_ERRCHK, entry,                                       \
            "   movem.l " save ",-(sp)\n"                                   \
            "   jsr     _" #cfunc "\n"                                      \
            "   movem.l (sp)+," save "\n"                                   \
            "   rts\n")

/* BLITZ_ERRCHK_MESSAGE(label, "text") - a runtime error message
   (BLITZ_RUNERR(label)) in the error check area, i.e. cut away with the
   checks when the debugger is off, as in the original.  Assembler data
   (in the function bl_errchk_message_label, BL_ASM_FUNC_): GCC refuses
   code and data in the same named section, so as soon as a unit has C
   check functions (BLITZ_ERRCHK) there, its messages cannot be C arrays
   with BLITZ_ERRCHK (units without check code may do that, see memlib).
   `text` is a C string literal without " or \ escapes. */
#define BLITZ_ERRCHK_MESSAGE(label, text)                                   \
    extern const char label[];                                              \
    BL_ASM_FUNC_(static BLITZ_ERRCHK, bl_errchk_message_##label,            \
            "_" #label ":\n   .asciz \"" text "\"\n   .even\n")

/* BLITZ_ERRCHK_AB(entry, blitz, amiga) - an error check with Blitz/Amiga
   variants in the error check area: `Dc $a001:Dc.l blitz` and then the
   Amiga mode check (JMP amiga), see BLITZ_AB_ROUTINE.  `blitz`/`amiga`
   are check entries (BLITZ_ERRCHK_THUNK or asm). */
#define BLITZ_ERRCHK_AB(entry, blitz, amiga)                                \
    BL_ASM_FUNC_(BLITZ_ERRCHK, entry,                                       \
            "   .word   0xa001\n"                                           \
            "   .long   _" #blitz "\n"                                      \
            "   jmp     _" #amiga "\n")

/* BLITZ_ERRCHK_ASM(name, "text") - assembler in the error check area: check
   routines that need assembler (a CCR input, the caller's return address,
   a CPU trap) and their error branches.  `text` brings its own labels
   (with .globl for the entries of the header) and ends with a return or
   a trap; `name` only names the block (the static function
   bl_errchk_asm_name, BL_ASM_FUNC_, which starts at the block's first
   byte).  A library writes no section directive of its own (#1057, the
   build checks it: cmake/check-asm-sections.cmake). */
#define BLITZ_ERRCHK_ASM(name, text)                                        \
    BL_ASM_FUNC_(static BLITZ_ERRCHK, bl_errchk_asm_##name, text "\n")

/* A routine in assembler, BLITZ_ASM_ROUTINE(entry, "text"): by nature (stack
   pointer, exception frames, hardware) or for speed on a hot path measured
   with the perf preset; say in a comment which. */
#define BLITZ_ASM_ROUTINE(entry, text) BL_ASM_FUNC_(, entry, text "\n")

/* BLITZ_ASM_BLOCK(name, "text") - assembler in the normal code that is not
   one routine with its entry first: handlers with BRA entries in front
   of their label, several routines, data in the code hunk.  `text` brings
   its own labels (and .globl); `name` only names the block (the static
   function bl_asm_name, BL_ASM_FUNC_, which starts at the block's first
   byte).  Like BLITZ_ERRCHK_ASM it replaces top-level asm with a section
   directive of its own (#1057). */
#define BLITZ_ASM_BLOCK(name, text)                                         \
    BL_ASM_FUNC_(static BLITZ_ROUTINE, bl_asm_##name, text "\n")

/* Inline routine (blitz2.s doajsr2): `Dc $a000,len` before the code;
   the compiler copies the `len` bytes to the call instead of a JSR.
   Inline code is by nature a few instructions, so it is given as
   assembler text; the final RTS is for the JSR case (constant folding). */
#define BLITZ_INLINE(entry, text)                                           \
    BL_ASM_FUNC_(, entry,                                                   \
            "   .word   0xa000,2f-1f\n"                                     \
            "1: " text "\n"                                                 \
            "2: rts\n")

/* Blitz/Amiga variants (`Dc $a001:Dc.l blitzcode` in front of the code):
   the compiler (blitz2.s doajsr2 .isone) calls `blitzcode` in Blitz mode
   and the code behind the 6 byte prefix (entry+6) in Amiga mode.
     BLITZ_AB_ROUTINE(entry, blitzentry, "text")
   `text` is the Amiga mode code, e.g. a thunk sequence that calls a C
   function; `blitzentry` is an ordinary routine (BLITZ_THUNK, C, asm). */
#define BLITZ_AB_ROUTINE(entry, blitzentry, text)                           \
    BL_ASM_FUNC_(, entry,                                                   \
            "   .word   0xa001\n"                                           \
            "   .long   _" #blitzentry "\n" text "\n")

/* Runtime error: `MOVE.l #message,d0:TRAP #0` - handled by the debugger
   (runtime error requester). */
#ifndef BL_CODE_MODEL
#define BLITZ_RUNERR(msg) do {                                              \
    register const char *_m __asm("d0") = (msg);                            \
    __asm volatile("trap #0" : : "d"(_m) : "memory");                       \
} while (0)
#else
#define BLITZ_RUNERR(msg) ((void)(msg))
#endif

#include "blitzhelp.h"  /* register arithmetic, a2 stack, mathffp LVOs, tokens */

#endif
