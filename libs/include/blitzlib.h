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
#if !defined(__clang__) && __STDC_HOSTED__
#error "compile Blitz libraries with -ffreestanding (see CMakeLists.txt)"
#endif
#ifndef __clang__
#pragma GCC optimize ("no-tree-loop-distribute-patterns")
#endif

#include <exec/types.h>
#include <exec/memory.h>
#include <libnums.h>     /* LIB_xxx = #xxx of libnums.bb; <>: a variant
                             (libs/variants) brings its own */

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

/* register codes for BLITZ_LIBS: value of lib `lib` into a register */
#define BL_LD(n)    ((n) << 8)              /* #ld0..#ld7 */
#define BL_LA(n)    (0x1000 | ((n) << 8))   /* #la0..#la3, #la6 */
#define BL_LPUSH    0xff00                  /* #lpush: onto the stack */
#define BL_BASE     1                       /* #base: item 0 of the objects */
#define BL_USED     2                       /* #used: current object */
#define BL_MAX      3                       /* #max: maximum setting */
#define BL_ITEM     0x80                    /* #item: object from param dN */
#define BL_PD(n)    (n)                     /* #pd0..#pd7 */

/* ------------------------------------------------------ fixed registers */

/* a5 = base of the Blitz program's globals (init results of all libs are
   globals at the header's $0A offset), a4 = locals of the current
   procedure, a3 = string work pointer (string results are written at a3,
   which then points to the terminating 0).  Declared as GCC global
   register variables; every unit that includes this header must be
   compiled with -ffixed-a3 -ffixed-a4 -ffixed-a5. */
#ifndef __clang__
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
#ifndef __clang__
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
   Dc -1:Dc.l toke,load,save,use,free:Dc 0,defmax,shift */
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
   called code may change every register except a3 (string pointer),
   a4, a5, a7 (Blitz convention), so all others are declared clobbered. */
#define BLITZ_ALIBJSR(tok, r) BL_JSR_(BL_TOKVAL(tok, 0), r)
#define BLITZ_ALIBJSR_FORM(tok, form, r) BL_JSR_(BL_TOKVAL(tok, form), r)
#define BLITZ_BLIBJSR(tok, r) BL_JSR_(BL_BTOKVAL(tok, 0), r)
#ifndef __clang__
#define BL_JSR_(val, r) ({                                                  \
    register ULONG _d0 __asm("d0") = (r).d0;                                \
    register ULONG _d1 __asm("d1") = (r).d1;                                \
    register ULONG _a0 __asm("a0") = (r).a0;                                \
    register ULONG _a1 __asm("a1") = (r).a1;                                \
    __asm volatile("jsr _blitz_tok+%c4"                                     \
        : "+d"(_d0), "+d"(_d1), "+a"(_a0), "+a"(_a1) : "i"(val)             \
        : "d2", "d3", "d4", "d5", "d6", "d7", "a2", "a6", "cc", "memory");  \
    (r).d0 = _d0; (r).d1 = _d1; (r).a0 = _a0; (r).a1 = _a1; _d0; })
#else
ULONG bl_jsr_clang(ULONG val, struct bl_regs *r);
#define BL_JSR_(val, r) bl_jsr_clang(val, &(r))
#endif

/* BLITZ_BASEJSR(base, off, r) - `JSR off(a6)` with a6 = base: call through
   a jump table / library base that the compiler passed in a register
   (!libs ...,#la6), e.g. memlib's membase table (JSR (a6) = localloc,
   JSR 4(a6) = locfree, +8 for the global versions) or a system library
   (mathffp: JSR -36(a6) = SPFlt).  In/out: r.d0/d1/a0/a1 as with
   BLITZ_ALIBJSR; the callee may change every register except a3-a5/a7
   (a6 included), so all others are declared clobbered.  (added by the
   mathstr group: strings2lib, maxlenlib, floatquicklib, modlib) */
#ifndef __clang__
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

/* BLITZ_BASEJSR1(base, off, x0) / BLITZ_BASEJSR2(base, off, x0, x1) -
   BLITZ_BASEJSR with one operand in d0 / two in d0 and d1 (the others 0),
   result d0.  The usual form for mathffp (a6 = ffplib base, !libs
   #ffplib,#la6): BLITZ_BASEJSR2(ffp, LVO_SPAdd, a, b). */
#define BLITZ_BASEJSR1(base, off, x0) ({                                    \
    struct bl_regs r_ = { .d0 = (x0) }; BLITZ_BASEJSR(base, off, r_); })
#define BLITZ_BASEJSR2(base, off, x0, x1) ({                                \
    struct bl_regs r_ = { .d0 = (x0), .d1 = (x1) };                         \
    BLITZ_BASEJSR(base, off, r_); })

/* BLITZ_ALIBJSR_A6(tok, base, x) - ALibJsr tok with a6 = `base`, in d0 =
   x, result d0.  BLITZ_ALIBJSR does not load a6 (and declares it
   clobbered), but some routines need a library base there, e.g.
   floatquicklib $D300 qutofl the ffplib base: the original calls it from
   a routine with !libs #ffplib,#la6 and a6 still loaded.  a6 is declared
   changed; a2 (the compiler's argument stack) is kept like by every
   library routine (see notes/cmdlibs-port.md), a3-a5/a7 anyway.  (from
   vallib Int) */
#ifndef __clang__
#define BLITZ_ALIBJSR_A6(tok, base, x) ({                                   \
    register ULONG _d0 __asm("d0") = (x);                                   \
    register const void *_a6 __asm("a6") = (base);                          \
    __asm volatile("jsr _blitz_tok+%c2"                                     \
        : "+d"(_d0), "+a"(_a6) : "i"(BL_TOKVAL(tok, 0))                     \
        : "d1", "d2", "d3", "d4", "d5", "d6", "d7", "a0", "a1", "cc", "memory"); \
    _d0; })
#else
#define BLITZ_ALIBJSR_A6(tok, base, x) ((void)(base), (ULONG)(x))
#endif

/* ALibJsr with d0 = x, a2 = a2v (argument stack for the callee, e.g. a
   string length or type word) and a6 = a6v; result d0.  The callee may
   change every register except a3-a5/a7. */
#ifndef __clang__
#define BLITZ_ALIBJSR_A2A6(tok, x, a2v, a6v) ({                             \
    register ULONG _d0 __asm("d0") = (ULONG)(x);                            \
    register const void *_a2 __asm("a2") = (a2v);                           \
    register const void *_a6 __asm("a6") = (a6v);                           \
    __asm volatile("jsr _blitz_tok+%c3"                                     \
        : "+d"(_d0), "+a"(_a2), "+a"(_a6) : "i"(BL_TOKVAL(tok, 0))          \
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
#ifndef __clang__
#define BLITZ_ALIBJSR_CCR(tok, r, cc) ({                                    \
    register ULONG _d0 __asm("d0") = (r).d0;                                \
    register ULONG _d1 __asm("d1") = (r).d1;                                \
    register ULONG _a0 __asm("a0") = (r).a0;                                \
    register ULONG _a1 __asm("a1") = (r).a1;                                \
    UBYTE _cc;                                                              \
    __asm volatile("jsr _blitz_tok+%c5\n\ts" cc " %4"                       \
        : "+d"(_d0), "+d"(_d1), "+a"(_a0), "+a"(_a1), "=m"(_cc)             \
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
#ifndef __clang__
#define BLITZ_ALIBJSR_CCR_D0A0(tok, cc, d0p, a0v) ({                        \
    register ULONG _d0 __asm("d0") = *(d0p);                                \
    register ULONG _d1 __asm("d1");                                         \
    register const void *_a0 __asm("a0") = (a0v);                           \
    register ULONG _a1 __asm("a1");                                         \
    UBYTE _cc;                                                              \
    __asm volatile("jsr _blitz_tok+%c5\n\ts" cc " %4"                       \
        : "+d"(_d0), "=d"(_d1), "+a"(_a0), "=a"(_a1), "=m"(_cc)             \
        : "i"(BL_TOKVAL(tok, 0))                                            \
        : "d2", "d3", "d4", "d5", "d6", "d7", "a2", "a6", "cc", "memory");  \
    *(d0p) = _d0; _cc; })
#else
#define BLITZ_ALIBJSR_CCR_D0A0(tok, cc, d0p, a0v) ((void)(a0v), (UBYTE)*(d0p))
#endif

/* BLITZ_ALIBJSR_D4(tok, d0, d1, d2, d3) / BLITZ_BLIBJSR_D4 - a routine of
   another library with its arguments in d0-d3 (BLITZ_ALIBJSR only passes
   d0/d1/a0/a1), e.g. spriteslib ShowSprite (sprite, x, y, channel).
   Result d0; the callee may change every register except a3-a5/a7. */
#ifndef __clang__
#define BL_JSR_D4_(val, x0, x1, x2, x3) ({                                  \
    register ULONG _d0 __asm("d0") = (x0);                                  \
    register ULONG _d1 __asm("d1") = (x1);                                  \
    register ULONG _d2 __asm("d2") = (x2);                                  \
    register ULONG _d3 __asm("d3") = (x3);                                  \
    __asm volatile("jsr _blitz_tok+%c4"                                     \
        : "+d"(_d0), "+d"(_d1), "+d"(_d2), "+d"(_d3) : "i"(val)             \
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
    void entry(void);                                                       \
    __asm__("   .text\n   .even\n   .globl  _" #entry "\n"                  \
            "_" #entry ":\n"                                                \
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
    void entry(void);                                                       \
    __asm__("   .text\n   .even\n   .globl  _" #entry "\n"                  \
            "_" #entry ":\n"                                                \
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
    void entry(void);                                                       \
    __asm__("   .section .blitzlib.errchk,\"ax\"\n"                         \
            "   .even\n   .globl  _" #entry "\n"                            \
            "_" #entry ":\n"                                                \
            "   movem.l " save ",-(sp)\n"                                   \
            "   jsr     _" #cfunc "\n"                                      \
            "   movem.l (sp)+," save "\n"                                   \
            "   rts\n"                                                      \
            "   .text\n")

/* BLITZ_ERRCHK_MESSAGE(label, "text") - a runtime error message
   (BLITZ_RUNERR(label)) in the error check area, i.e. cut away with the
   checks when the debugger is off, as in the original.  Assembler data:
   GCC refuses code and data in the same named section, so as soon as a
   unit has C check functions (BLITZ_ERRCHK) there, its messages cannot be
   C arrays with BLITZ_ERRCHK (units without check code may do that, see
   memlib).  `text` is a C string literal without " or \ escapes. */
#define BLITZ_ERRCHK_MESSAGE(label, text)                                   \
    extern const char label[];                                              \
    __asm__("   .section .blitzlib.errchk,\"ax\"\n"                         \
            "_" #label ":\n   .asciz \"" text "\"\n   .even\n   .text\n")

/* BLITZ_ERRCHK_AB(entry, blitz, amiga) - an error check with Blitz/Amiga
   variants in the error check area: `Dc $a001:Dc.l blitz` and then the
   Amiga mode check (JMP amiga), see BLITZ_AB_ROUTINE.  `blitz`/`amiga`
   are check entries (BLITZ_ERRCHK_THUNK or asm). */
#define BLITZ_ERRCHK_AB(entry, blitz, amiga)                                \
    void entry(void);                                                       \
    __asm__("   .section .blitzlib.errchk,\"ax\"\n"                         \
            "   .even\n   .globl  _" #entry "\n"                            \
            "_" #entry ":\n"                                                \
            "   .word   0xa001\n"                                           \
            "   .long   _" #blitz "\n"                                      \
            "   jmp     _" #amiga "\n"                                      \
            "   .text\n")

/* A routine that is assembler by nature (stack pointer manipulation,
   exception frames, hardware): BLITZ_ASM_ROUTINE(entry, "text").  Keep the
   original instructions and say in a comment why C cannot do it. */
#define BLITZ_ASM_ROUTINE(entry, text)                                      \
    void entry(void);                                                       \
    __asm__("   .text\n   .even\n   .globl  _" #entry "\n"                  \
            "_" #entry ":\n" text "\n")

/* Inline routine (Compiler-Spec 9.3.2): `Dc $a000,len` before the code;
   the compiler copies the `len` bytes to the call instead of a JSR.
   Inline code is by nature a few instructions, so it is given as
   assembler text; the final RTS is for the JSR case (constant folding). */
#define BLITZ_INLINE(entry, text)                                           \
    void entry(void);                                                       \
    __asm__("   .text\n   .even\n   .globl  _" #entry "\n"                  \
            "_" #entry ":\n"                                                \
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
    void entry(void);                                                       \
    __asm__("   .text\n   .even\n   .globl  _" #entry "\n"                  \
            "_" #entry ":\n"                                                \
            "   .word   0xa001\n"                                           \
            "   .long   _" #blitzentry "\n" text "\n")

/* Runtime error: `MOVE.l #message,d0:TRAP #0` - handled by the debugger
   (runtime error requester). */
#ifndef __clang__
#define BLITZ_RUNERR(msg) do {                                              \
    register const char *_m __asm("d0") = (msg);                            \
    __asm volatile("trap #0" : : "d"(_m) : "memory");                       \
} while (0)
#else
#define BLITZ_RUNERR(msg) ((void)(msg))
#endif

#include "blitzhelp.h"  /* register arithmetic, a2 stack, mathffp LVOs, tokens */

#endif
