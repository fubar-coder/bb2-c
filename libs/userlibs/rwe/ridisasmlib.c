/*
 * ridisasmlib - RWE userlib 50 (#ridisasmlib, Leading Edge Software
 * 1994): a table driven 680x0 disassembler - DisAssemble$(address) and
 * InstrSize.
 *
 * C translation of rwelibsrc/Disasmlib/Disasm-library.bb2, the last
 * original version (the AmiBlitz3 RIDisAsm.obj is an AmiBlitz3 derivative
 * that calls THOR's disassembler.library first: docs/rwelibs/README.md
 * 3.2); the reference is the reference build of the source by blitz2
 * 2.10 (ref/ridisasmlib.obj): the declarations equal it (ctest
 * rwe_declarations_ridisasmlib), the behaviour is compared with it in
 * libs/tests/rwe/ridisasmlib.c.  Notes: docs/rwelibs/port/ridisasmlib.md.
 * The source needs no library at runtime (init and finit are RTS).
 *
 * DisAssemble$(address): the first instruction word is matched against
 * the 126 entries of DisData (mask, compare value, name, source and
 * destination <ea> mask/shift or a special code, size mask/shift); the
 * line is built in LineBuff: the name (with condition codes for "*", s/u
 * for "!" and the size) in columns 0-8, the operands from column 9; the
 * string is always the first 34 columns (spaces after the operands, cut
 * after 34), written to a3 with 17 word writes; InstrSize = the bytes
 * the instruction took.  An instruction not in the table gives "?????"
 * (length 5) and InstrSize 2.
 *
 * Everything is C.  The data around LineBuff keep the original's order
 * and sizes (the code bytes in front of it, LineBuff, the five words, the
 * condition tables), so that a line longer than 80 bytes and the look at
 * the bytes in front of a short name hit the same things.  The original's
 * register juggling is kept where it decides something: the operand
 * pointer a5 (docomma compares it with a2 - see BUG(orig) (#615)).  The memory
 * is read with the original's access widths (word/long reads at the
 * given, possibly odd, address).
 *
 * BUG(orig):
 *   - A decimal 0 (a displacement 0 of d16(an), brief and full format
 *     extensions, bd/od 0 or suppressed) is not "0": DecPrint has
 *     `LEA .l8,a0` for its local label 'l8, which blitz2 2.10 assembles
 *     as the absolute address 8 - the line gets the bytes of the bus
 *     error vector up to a 0 byte (on Kickstart 3.2 in the emulator
 *     "@J(" ...) (#614).
 *   - docomma (a comma between the operands) is "a2 <> a5" after the
 *     source operand, but a5 is also the number/register list pointer of
 *     the operand routines: a source that only moves a5 (MOVEM with an
 *     empty register list) still gets a comma (#615).
 *   - The string is cut after 34 columns: longer operands are lost; a line
 *     can grow to 86 bytes (two memory indirect <ea>s with 32 bit
 *     displacements) and overwrite opsize, opextra and extrainstr (#616).
 *   - DisAssemble$ writes words to a3: an odd a3 is an address error on a
 *     68000 (here as well), like the instruction reads at an odd address
 *     (#617).
 */
#include "blitzlib.h"
#include <stddef.h>

/* entries */
void disoneline(void), instrsize(void);
void libinit(void), libfinit(void);

BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(init);

/* !libheader {#ridisasmlib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_ridisasmlib, init, BL_NORET, finit, 0);

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(disoneline, 0, 0);
BLITZ_NAME("DisAssemble$", "(address.l)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(instrsize, 0, 0);
BLITZ_NAME("InstrSize", "- returns size of last instruction disassembled");

BLITZ_NULLSUB_AT(finit, libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, libinit, 0, 0);

BLITZ_LIBFIN();

/* the codes for special operands (an <ea> mask of 0) */
enum { IMMED, NONE, RRRMMM11, TOCCR, TOSR, DESTADDR11, DESTDATA11, QIMMED11, DESTDATA2,
       DISPLACE, IMMED2, DAREG215, ADDRPI2, ADDRPI11, ADDRPD2, ADDRPD11, DISPLACE2,
       _DATA214202, DESTADDR2, TUSP, CONTROL2, RLIST, BYTE7, VNUM };
#define STDEA   0b0000000000111111
#define STDSIZE 0b0000000011000000

/* an entry of DisData: an instruction matches if (op & mask) == cmp;
   <ea> mask 0: the shift is a special code; size mask 0: the shift is
   the size; bits 12-15 of the size shift: extra instruction words (the
   first is read into extrainstr) */
struct disdata {
    UWORD mask, cmp;
    char name[8];
    UWORD srcmask, src, dstmask, dst, szmask, szshift;
};
_Static_assert(sizeof(struct disdata) == 24, "DisData entry");

static const struct disdata disdata[] = {
    { 0b1111000000000000, 0b0001000000000000, "move.b",
      0b0000000000111111, 0, 0, RRRMMM11, 0, 0b00 },
    { 0b1111000000000000, 0b0010000000000000, "move.l",
      0b0000000000111111, 0, 0, RRRMMM11, 0, 0b10 },
    { 0b1111000000000000, 0b0011000000000000, "move.w",
      0b0000000000111111, 0, 0, RRRMMM11, 0, 0b01 },
    { 0b1111111100000000, 0b0000001000000000, "andi.",
      0, IMMED, 0b0000000000111111, 0, 0b0000000011000000, 6 },
    { 0b1111111100000000, 0b0000000000000000, "ori.",
      0, IMMED, 0b0000000000111111, 0, 0b0000000011000000, 6 },
    { 0b1111111100000000, 0b0000010000000000, "subi.",
      0, IMMED, 0b0000000000111111, 0, 0b0000000011000000, 6 },
    { 0b1111111100000000, 0b0000011000000000, "addi.",
      0, IMMED, 0b0000000000111111, 0, 0b0000000011000000, 6 },
    { 0b1111111100000000, 0b0000101000000000, "eori.",
      0, IMMED, 0b0000000000111111, 0, 0b0000000011000000, 6 },
    { 0b1111111100000000, 0b0000110000000000, "cmpi.",
      0, IMMED, 0b0000000000111111, 0, 0b0000000011000000, 6 },
    { 0b1111111111111111, 0b0000000000111100, "ori.",
      0, IMMED, 0, TOCCR, 0, 0b01 },
    { 0b1111111111111111, 0b0000000001111100, "ori.",
      0, IMMED, 0, TOSR, 0, 0b01 },
    { 0b1111111111111111, 0b0000001000111100, "andi.",
      0, IMMED, 0, TOCCR, 0, 0b01 },
    { 0b1111111111111111, 0b0000001001111100, "andi.",
      0, IMMED, 0, TOSR, 0, 0b01 },
    { 0b1111111111111111, 0b0000101000111100, "eori.",
      0, IMMED, 0, TOCCR, 0, 0b01 },
    { 0b1111111111111111, 0b0000101001111100, "eori.",
      0, IMMED, 0, TOSR, 0, 0b01 },
    { 0b1111111111000000, 0b0100000011000000, "move",
      0, TOSR, 0b111111, 0, 0, 0b01 },
    { 0b1111111111000000, 0b0100001011000000, "move",
      0, TOCCR, 0b111111, 0, 0, 0b01 },
    { 0b1111111111000000, 0b0100010011000000, "move",
      0b111111, 0, 0, TOCCR, 0, 0b01 },
    { 0b1111111111000000, 0b0100011011000000, "move",
      0b111111, 0, 0, TOSR, 0, 0b01 },
    { 0b1111111111111000, 0b0100100001000000, "swap",
      0b0000000000111111, 0, 0, NONE, 0, 0 },
    { 0b1111111111000000, 0b0100111011000000, "jmp",
      0b0000000000111111, 0, 0, NONE, 0, 0 },
    { 0b1111111111000000, 0b0100111010000000, "jsr",
      0b0000000000111111, 0, 0, NONE, 0, 0 },
    { 0b1111111111000000, 0b0100101011000000, "tas",
      0b0000000000111111, 0, 0, NONE, 0, 0 },
    { 0b1111111111111000, 0b0100100000001000, "link.l",
      0, DESTADDR2, 0, IMMED, 0, 0b10 },
    { 0b1111111111000000, 0b0100100000000000, "nbcd",
      0b0000000000111111, 0, 0, NONE, 0, 0 },
    { 0b1111111100000000, 0b0100101000000000, "tst.",
      0b0000000000111111, 0, 0, NONE, 0b0000000011000000, 6 },
    { 0b1111111111111000, 0b0100100001001000, "bkpt",
      0, IMMED2, 0, NONE, 0, 0 },
    { 0b1111111111000000, 0b0100100001000000, "pea",
      0b0000000000111111, 0, 0, NONE, 0, 0 },
    { 0b1111111111111000, 0b0100100010000000, "ext.w",
      0b0000000000111111, 0, 0, NONE, 0, 0 },
    { 0b1111111111111000, 0b0100100011000000, "ext.l",
      0b0000000000111111, 0, 0, NONE, 0, 0 },
    { 0b1111111100000000, 0b0100011000000000, "not.",
      0b0000000000111111, 0, 0, NONE, 0b0000000011000000, 6 },
    { 0b1111111100000000, 0b0100010000000000, "neg.",
      0b0000000000111111, 0, 0, NONE, 0b0000000011000000, 6 },
    { 0b1111111100000000, 0b0100000000000000, "negx.",
      0b0000000000111111, 0, 0, NONE, 0b0000000011000000, 6 },
    { 0b1111111100000000, 0b0100001000000000, "clr.",
      0b0000000000111111, 0, 0, NONE, 0b0000000011000000, 6 },
    { 0b1111111111111111, 0b0100101011111100, "illegal",
      0, NONE, 0, NONE, 0, 0 },
    { 0b1111111111111111, 0b0100111001110000, "reset",
      0, NONE, 0, NONE, 0, 0 },
    { 0b1111111111111111, 0b0100111001110001, "nop",
      0, NONE, 0, NONE, 0, 0 },
    { 0b1111111111111111, 0b0100111001110010, "stop",
      0, IMMED, 0, NONE, 0, 0b01 },
    { 0b1111111111111111, 0b0100111001110011, "rte",
      0, NONE, 0, NONE, 0, 0 },
    { 0b1111111111111111, 0b0100111001110101, "rts",
      0, NONE, 0, NONE, 0, 0 },
    { 0b1111111111111111, 0b0100111001110110, "trapv",
      0, NONE, 0, NONE, 0, 0 },
    { 0b1111111111111111, 0b0100111001110111, "rtr",
      0, NONE, 0, NONE, 0, 0 },
    { 0b1111111111111000, 0b0100100111000000, "extb.l",
      0, DESTDATA2, 0, NONE, 0, 0 },
    { 0b1111000111000000, 0b0100000111000000, "lea",
      0b0000000000111111, 0, 0, DESTADDR11, 0, 0 },
    { 0b1111000111000000, 0b0010000001000000, "movea.l",
      0b0000000000111111, 0, 0, DESTADDR11, 0, 0b10 },
    { 0b1111000111000000, 0b0011000001000000, "movea.w",
      0b0000000000111111, 0, 0, DESTADDR11, 0, 0b01 },
    { 0b1111000011000000, 0b1101000011000000, "adda.",
      STDEA, 0, 0, DESTADDR11, 0b0000000100000000, 8 },
    { 0b1111000100000000, 0b1101000000000000, "add.",
      STDEA, 0, 0, DESTDATA11, STDSIZE, 6 },
    { 0b1111000100000000, 0b1101000100000000, "add.",
      0, DESTDATA11, STDEA, 0, STDSIZE, 6 },
    { 0b1111000100000000, 0b0101000000000000, "addq.",
      0, QIMMED11, STDEA, 0, STDSIZE, 6 },
    { 0b1111000100000000, 0b1100000000000000, "and.",
      STDEA, 0, 0, DESTDATA11, STDSIZE, 6 },
    { 0b1111000111111000, 0b1100000101000000, "exg",
      0, DESTDATA11, 0, DESTDATA2, 0, 0 },
    { 0b1111000111111000, 0b1100000101001000, "exg",
      0, DESTADDR11, 0, DESTADDR2, 0, 0 },
    { 0b1111000111111000, 0b1100000110001000, "exg",
      0, DESTDATA11, 0, DESTADDR2, 0, 0 },
    { 0b1111000100000000, 0b1100000100000000, "and.",
      0, DESTDATA11, STDEA, 0, STDSIZE, 6 },
    { 0b1111000100111000, 0b1110000100100000, "asl.",
      0, DESTDATA11, 0, DESTDATA2, STDSIZE, 6 },
    { 0b1111000100111000, 0b1110000100000000, "asl.",
      0, QIMMED11, 0, DESTDATA2, STDSIZE, 6 },
    { 0b1111111111000000, 0b1110000111000000, "asl.w",
      0, NONE, STDEA, 0, 0, 0b01 },
    { 0b1111000100111000, 0b1110000000100000, "asr.",
      0, DESTDATA11, 0, DESTDATA2, STDSIZE, 6 },
    { 0b1111000100111000, 0b1110000000000000, "asr.",
      0, QIMMED11, 0, DESTDATA2, STDSIZE, 6 },
    { 0b1111111111000000, 0b1110000011000000, "asr.w",
      0, NONE, STDEA, 0, 0, 0b01 },
    { 0b1111000000000000, 0b0110000000000000, "b*",
      0, NONE, 0, DISPLACE, 0, 0 },
    { 0b1111000111000000, 0b0000000101000000, "bchg",
      0, DESTDATA11, STDEA, 0, 0, 0 },
    { 0b1111111111000000, 0b0000100001000000, "bchg",
      0, IMMED, STDEA, 0, 0, 0b00 },
    { 0b1111000111000000, 0b0000000110000000, "bclr",
      0, DESTDATA11, STDEA, 0, 0, 0 },
    { 0b1111111111000000, 0b0000100010000000, "bclr",
      0, IMMED, STDEA, 0, 0, 0b00 },
    { 0b1111000111000000, 0b0000000111000000, "bset",
      0, DESTDATA11, STDEA, 0, 0, 0 },
    { 0b1111111111000000, 0b0000100011000000, "bset",
      0, IMMED, STDEA, 0, 0, 0b00 },
    { 0b1111000111000000, 0b0000000100000000, "btst",
      0, DESTDATA11, STDEA, 0, 0, 0 },
    { 0b1111111111000000, 0b0000100000000000, "btst",
      0, IMMED, STDEA, 0, 0, 0b00 },
    { 0b1111111110000000, 0b0100100010000000, "movem.",
      0, RLIST, STDEA, 0, 0b0000000001000000, 6+0x1000 },
    { 0b1111111110000000, 0b0100110010000000, "movem.",
      STDEA, 0, 0, RLIST, 0b0000000001000000, 6+0x1000 },
    { 0b1111111111000000, 0b0100110000000000, "mul!.l",
      STDEA, 0, 0, _DATA214202, 0, 0b10+0x1000 },
    { 0b1111000001000000, 0b0100000000000000, "chk.",
      STDEA, 0, 0, DESTDATA11, 0b0000000110000000, 7 },
    { 0b1111100111000000, 0b0000000011000000, "chk2.",
      STDEA, 0, 0, DAREG215, 0b0000011000000000, 9+0x1000 },
    { 0b1111000100000000, 0b1011000000000000, "cmp.",
      STDEA, 0, 0, DESTDATA11, STDSIZE, 6 },
    { 0b1111000011000000, 0b1011000011000000, "cmpa.",
      STDEA, 0, 0, DESTADDR11, 0b0000000100000000, 8 },
    { 0b1111000100111000, 0b1011000100001000, "cmpm.",
      0, ADDRPI2, 0, ADDRPI2, STDSIZE, 6 },
    { 0b1111100111000000, 0b0000000011000000, "cmp2.",
      STDEA, 0, 0, DAREG215, 0b11000000000, 9+0x1000 },
    { 0b1111000011111000, 0b0101000011001000, "db*",
      0, DESTDATA2, 0, DISPLACE2, 0, 0 },
    { 0b1111000111000000, 0b1000000111000000, "divs.w",
      STDEA, 0, 0, DESTDATA11, 0, 0b01 },
    { 0b1111111111000000, 0b0100110001000000, "div!.l",
      STDEA, 0, 0, _DATA214202, 0, 0b10+0x1000 },
    { 0b1111000111000000, 0b1000000011000000, "divu.w",
      STDEA, 0, 0, DESTDATA11, 0, 0b01 },
    { 0b1111000100000000, 0b1011000100000000, "eor.",
      0, DESTDATA11, STDEA, 0, STDSIZE, 6 },
    { 0b1111111111111000, 0b0100111001010000, "link.w",
      0, DESTADDR2, 0, IMMED, 0, 0b01 },
    { 0b1111000100111000, 0b1110000100101000, "lsl.",
      0, DESTDATA11, 0, DESTDATA2, STDSIZE, 6 },
    { 0b1111000100111000, 0b1110000100001000, "lsl.",
      0, QIMMED11, 0, DESTDATA2, STDSIZE, 6 },
    { 0b1111111111000000, 0b1110001111000000, "lsl",
      0, NONE, STDEA, 0, 0, 0 },
    { 0b1111000100111000, 0b1110000000101000, "lsr.",
      0, DESTDATA11, 0, DESTDATA2, STDSIZE, 6 },
    { 0b1111000100111000, 0b1110000000001000, "lsr.",
      0, QIMMED11, 0, DESTDATA2, STDSIZE, 6 },
    { 0b1111111111000000, 0b1110001011000000, "lsr",
      0, NONE, STDEA, 0, 0, 0 },
    { 0b1111111111111000, 0b0100111001101000, "move",
      0, DESTADDR2, 0, TUSP, 0, 0 },
    { 0b1111111111111000, 0b0100111001100000, "move",
      0, TUSP, 0, DESTADDR2, 0, 0 },
    { 0b1111111111111111, 0b0100111001111010, "movec",
      0, CONTROL2, 0, DAREG215, 0, 0+0x1000 },
    { 0b1111111111111111, 0b0100111001111011, "movec",
      0, DAREG215, 0, CONTROL2, 0, 0+0x1000 },
    { 0b1111000100000000, 0b0111000000000000, "moveq",
      0, BYTE7, 0, DESTDATA11, 0, 0 },
    { 0b1111000111000000, 0b1100000111000000, "muls.w",
      STDEA, 0, 0, DESTDATA11, 0, 0b01 },
    { 0b1111000111000000, 0b1100000011000000, "mulu.w",
      STDEA, 0, 0, DESTDATA11, 0, 0b01 },
    { 0b1111000100111000, 0b1110000100111000, "rol.",
      0, DESTDATA11, 0, DESTDATA2, STDSIZE, 6 },
    { 0b1111000100111000, 0b1110000100011000, "rol.",
      0, QIMMED11, 0, DESTDATA2, STDSIZE, 6 },
    { 0b1111111111000000, 0b1110011111000000, "rol.w",
      0, NONE, STDEA, 0, 0, 0b01 },
    { 0b1111000100111000, 0b1110000000111000, "ror.",
      0, DESTDATA11, 0, DESTDATA2, STDSIZE, 6 },
    { 0b1111000100111000, 0b1110000000011000, "ror.",
      0, QIMMED11, 0, DESTDATA2, STDSIZE, 6 },
    { 0b1111111111000000, 0b1110011011000000, "ror.w",
      0, NONE, STDEA, 0, 0, 0b01 },
    { 0b1111000100111000, 0b1110000100110000, "roxl.",
      0, DESTDATA11, 0, DESTDATA2, STDSIZE, 6 },
    { 0b1111000100111000, 0b1110000100010000, "roxl.",
      0, QIMMED11, 0, DESTDATA2, STDSIZE, 6 },
    { 0b1111111111000000, 0b1110010111000000, "roxl.w",
      0, NONE, STDEA, 0, 0, 0b01 },
    { 0b1111000100111000, 0b1110000000110000, "roxr.",
      0, DESTDATA11, 0, DESTDATA2, STDSIZE, 6 },
    { 0b1111000100111000, 0b1110000000010000, "roxr.",
      0, QIMMED11, 0, DESTDATA2, STDSIZE, 6 },
    { 0b1111111111000000, 0b1110010011000000, "roxr.w",
      0, NONE, STDEA, 0, 0, 0b01 },
    { 0b1111111111111111, 0b0100111001110100, "rtd",
      0, IMMED, 0, NONE, 0, 0b01 },
    { 0b1111000111111000, 0b1000000100000000, "sbcd",
      0, DESTDATA2, 0, DESTDATA11, 0, 0 },
    { 0b1111000111111000, 0b1000000100001000, "sbcd",
      0, ADDRPD2, 0, ADDRPD11, 0, 0 },
    { 0b1111000111111000, 0b1100000100000000, "abcd",
      0, DESTDATA2, 0, DESTDATA11, 0, 0 },
    { 0b1111000111111000, 0b1100000100001000, "abcd",
      0, ADDRPD2, 0, ADDRPD11, 0, 0 },
    { 0b1111000100111000, 0b1001000100000000, "subx.",
      0, DESTDATA2, 0, DESTDATA11, STDSIZE, 6 },
    { 0b1111000100111000, 0b1001000100001000, "subx.",
      0, ADDRPD2, 0, ADDRPD11, STDSIZE, 6 },
    { 0b1111000011111111, 0b0101000011111100, "trap*",
      0, NONE, 0, NONE, 0, 0 },
    { 0b1111000011111100, 0b0101000011111000, "trap*",
      0, IMMED, 0, NONE, 0b0000000000000011, 0 },
    { 0b1111000011000000, 0b0101000011000000, "s*",
      0, NONE, STDEA, 0, 0, 0 },
    { 0b1111000011000000, 0b1001000011000000, "suba.",
      STDEA, 0, 0, DESTADDR11, 0b0000000100000000, 8 },
    { 0b1111000100000000, 0b1001000000000000, "sub.",
      STDEA, 0, 0, DESTDATA11, STDSIZE, 6 },
    { 0b1111000100000000, 0b1001000100000000, "sub.",
      0, DESTDATA11, STDEA, 0, STDSIZE, 6 },
    { 0b1111000100000000, 0b0101000100000000, "subq.",
      0, QIMMED11, STDEA, 0, STDSIZE, 6 },
    { 0b1111111111110000, 0b0100111001000000, "trap",
      0, VNUM, 0, NONE, 0, 0 },
    { 0b1111111111111000, 0b0100111001011000, "unlk",
      0, NONE, 0, DESTADDR2, 0, 0 },
    { 0 }                               /* END OF DATA */
};

/* LineBuff and what follows it, in the original's order; in front of it
   the last bytes of DisOneLine (MOVEQ #34,d0 : RTS) */
static struct BL_PACKED {
    UBYTE code[4];
    char linebuff[80];
    UWORD opsize, opextra, extrainstr, isdestop, docomma;
    char condtable[32] __attribute__((nonstring));
    char bcondtable[32] __attribute__((nonstring));
} v = {
    { 0x70, 0x22, 0x4e, 0x75 }, "", 0, 0, 0, 0, 0,
    "t f hilscccsneeqvcvsplmigeltgtle",
    "rasrhilscccsneeqvcvsplmigeltgtle"
};
_Static_assert(offsetof(__typeof__(v), opsize) == 84, "LineBuff layout");

static ULONG disasm_add;
static const UBYTE *instr;              /* a0: the instruction stream */
static char *linepos;                   /* a2: the line */
static const char *opstr;               /* a5: the operand string / list */

static char hexbuff2[11] = "$00000000"; /* "$" HexBuff "00000000",0,0 */
static const char hexchrtab[] = "0123456789ABCDEF";
static struct {
    char prefix[2] __attribute__((nonstring));  /* 'l6: "#","-" */
    char digits[16];                    /* 'l7 */
} dec = { "#-", "" };
/* DecPrint's "0": LEA .l8,a0 - the label is 'l8, blitz2 2.10 assembles
   the absolute address 8 (no relocation): the bus error vector */
#define DECZERO ((char *)8)
static const char scaletab[] __attribute__((nonstring)) = "0248";
static const char pdrlist[32] __attribute__((nonstring)) =
    "d0d1d2d3d4d5d6d7a0a1a2a3a4a5a6a7";
static char pdrbuff[50];
static char rangereg, rangedigit;     /* the pending end of a register range */

/* the memory, with the original's access widths */
static UWORD peekw(void)
{
    return *(volatile const UWORD *)instr;
}

static UWORD rdw(void)
{
    UWORD w = peekw();

    instr += 2;
    return w;
}

static ULONG rdl(void)
{
    ULONG l = *(volatile const ULONG *)instr;

    instr += 4;
    return l;
}

static void put(char c)
{
    *linepos++ = c;
}

/* LSR.w dn,dm: the count modulo 64, 16 and more give 0 */
static UWORD lsrw(UWORD x, UWORD n)
{
    n &= 63;
    return n >= 16 ? 0 : (UWORD)(x >> n);
}

/* HexPrint: "$" and 8 digits */
static char *hexprint(ULONG n)
{
    int i;

    for (i = 0; i < 8; i++)
        hexbuff2[1 + i] = hexchrtab[(n >> (28 - 4 * i)) & 15];
    return hexbuff2;
}

/* DecPrint: unsigned decimal (subtracting the powers of ten) */
static char *decprint(ULONG n)
{
    static const ULONG pow[10] = { 1000000000, 100000000, 10000000, 1000000, 100000,
                                   10000, 1000, 100, 10, 1 };
    char *p = dec.digits;
    int i, started = 0;

    if (!n)
        return DECZERO;                 /* BUG(orig) (#614) */
    *p = '0';
    for (i = 0; i < 10; i++) {
        if (!started) {
            if (n < pow[i])
                continue;
            started = 1;
        }
        while (n >= pow[i]) {
            n -= pow[i];
            (*p)++;
        }
        *++p = '0';
    }
    *p = 0;
    return dec.digits;
}

/* SDecPrint: signed decimal, the "-" in front of digits ('l7) */
static char *sdecprint(ULONG n)
{
    char *p;

    if ((LONG)n >= 0)
        return decprint(n);
    p = decprint(0xffffffffUL - n + 1);
    return p - 1;
}

/* STRCOPY: opstr (a5) to linepos (a2), opstr behind the 0 */
static void strcopy(void)
{
    char c;

    while ((c = *opstr++) != 0)
        put(c);
}

/* HexPrint with only the last `digits` digits: "$" over the one before */
static void hexlast(ULONG n, int digits)
{
    char *p = hexprint(n) + 8 - digits;

    *p = '$';
    opstr = p;
    strcopy();
}

static void error6(void)
{
    int i;

    for (i = 0; i < 6; i++)
        put('?');
}

/* #immediate of opsize: long, word ($XXXX) or byte ($XX of the word) */
static void immediate(void)
{
    ULONG n = v.opsize == 2 ? rdl() : rdw();
    char *p;

    put('#');
    p = hexprint(n);
    if (v.opsize != 2) {
        p += v.opsize == 1 ? 4 : 6;
        *p = '$';
    }
    opstr = p;
    strcopy();
}

/* an index register of an extension word: "d5.w" "*2" */
static void indexreg(UWORD ext, char reg, UWORD scale)
{
    *linepos = (ext & 0x8000) ? 'a' : 'd';
    linepos++;
    put(reg);
    put('.');
    *linepos = (ext & 0x800) ? 'l' : 'w';
    linepos++;
    if (scale) {
        put('*');
        put(scaletab[scale]);
    }
}

/* the base register of a full format: "a"reg or "pc" */
static void basereg(char reg, int pc)
{
    if (pc) {
        put('p');
        put('c');
    } else {
        put('a');
        put(reg);
    }
}

/* a base or outer displacement of size sz (1 = none, 2 = word, 3 = long) */
static ULONG disp(UWORD sz)
{
    if (sz == 2)
        return (ULONG)(LONG)(WORD)rdw();
    if (sz == 3)
        return rdl();
    return 0;
}

/* mode 6 / mode 7 reg 3: the extension word formats; reg = the address
   register digit (pc: the PC) */
static void indexed(char reg, int pc)
{
    const UBYTE *at = instr;            /* d6: the extension word's address */
    UWORD e = peekw(), scale;
    char ireg;
    ULONG n;

    if (!(e & 0x100)) {
        /* brief: d8(an,xn.s*scale) / address(pc,xn.s*scale) */
        e = rdw();
        scale = (e & 0x600) >> 9;
        ireg = (char)('0' + ((e & 0x7000) >> 12));
        n = (ULONG)(LONG)(BYTE)e;
        if (pc)
            opstr = hexprint(n + (ULONG)at);
        else
            opstr = sdecprint(n);
        strcopy();
        put('(');
        basereg(reg, pc);
        put(',');
        indexreg(e, ireg, scale);
        put(')');
        return;
    }
    if ((e & 4) && !(e & 3)) {
        error6();                       /* no format matches */
        return;
    }
    e = rdw();
    scale = (e & 0x600) >> 9;
    ireg = (char)('0' + ((e & 0x7000) >> 12));
    if (!((e & 0x30) >> 4)) {
        error6();                       /* base displacement size 0 */
        return;
    }
    n = disp((e & 0x30) >> 4);
    if (!(e & 3)) {
        /* bd(an,xn) without memory indirection */
        opstr = sdecprint(n);
        strcopy();
        put('(');
        if (!(e & 0x80))
            basereg(reg, pc);
        if (!(e & 0x40)) {
            if (linepos[-1] != '(')
                put(',');
            indexreg(e, ireg, scale);
        }
        put(')');
        return;
    }
    /* memory indirect: ([bd,an],xn,od) (bit 2) or ([bd,an,xn],od) */
    put('(');
    put('[');
    opstr = sdecprint(n);
    strcopy();
    if (!(e & 0x80)) {
        put(',');
        basereg(reg, pc);
    }
    if (e & 4)
        put(']');
    if (!(e & 0x40)) {
        put(',');
        indexreg(e, ireg, scale);
    }
    if (!(e & 4))
        put(']');
    put(',');
    n = disp(e & 3);
    opstr = sdecprint(n);
    strcopy();
    put(')');
}

/* DecodeEARev: mode, register */
static void decodeearev(UWORD mode, UWORD r)
{
    char reg = (char)('0' + r);
    const UBYTE *at;
    ULONG n;

    switch (mode) {
    case 0:
        put('d');
        put(reg);
        return;
    case 1:
        put('a');
        put(reg);
        return;
    case 2:
    case 3:
        put('(');
        put('a');
        put(reg);
        put(')');
        if (mode == 3)
            put('+');
        return;
    case 4:
        put('-');
        put('(');
        put('a');
        put(reg);
        put(')');
        return;
    case 5:
        opstr = sdecprint((ULONG)(LONG)(WORD)rdw());
        strcopy();
        put('(');
        put('a');
        put(reg);
        put(')');
        return;
    case 6:
        indexed(reg, 0);
        return;
    }
    switch (r) {                        /* mode 7 */
    case 2:
        at = instr;
        n = (ULONG)(LONG)(WORD)rdw() + (ULONG)at;
        opstr = hexprint(n);
        strcopy();
        put('(');
        put('p');
        put('c');
        put(')');
        return;
    case 3:
        indexed(reg, 1);
        return;
    case 4:
        immediate();
        return;
    case 0:
        hexlast(rdw(), 4);
        put('.');
        put('w');
        return;
    case 1:
        opstr = hexprint(rdl());
        strcopy();
        return;
    }
    error6();
}

/* DecodeEA: mode/register as in the instruction */
static void decodeea(UWORD ea)
{
    decodeearev((ea & 0x38) >> 3, ea & 7);
}

/* the register list of MOVEM (extrainstr) */
static void rlist(UWORD op)
{
    UWORD list = v.extrainstr, r = 0, last;
    const char *p = pdrlist;
    char *b = pdrbuff;
    int i;

    if ((op & 0x38) >> 3 != 4) {        /* not -(an): bit 0 = d0, reverse */
        for (i = 0; i < 16; i++) {
            r = (UWORD)((r >> 1) | (list & 0x8000));
            list <<= 1;
        }
        list = r;
    }
    for (i = 15; i >= 0; i--) {         /* bit 15 = d0 ... bit 0 = a7 */
        if (list & (1U << i)) {
            *b++ = p[0];
            *b++ = p[1];
        }
        p += 2;
    }
    *b = 0;
    opstr = p;
    rangereg = 0;
    last = 9999;
    for (b = pdrbuff; *b; ) {
        char c = *b++, digit = *b++;
        UWORD code = (UWORD)((((UWORD)(UBYTE)c - 'a') & 1) << 4);
        WORD dist;

        code += (UWORD)((UBYTE)digit - '0');    /* a0-a7 0-7, d0-d7 16-23 */
        dist = (WORD)(last - code);
        if (dist < 0)
            dist = (WORD)-dist;
        if (dist <= 1) {
            last = code;
            rangereg = c;
            rangedigit = digit;
            if (linepos[-1] != '-')
                put('-');
            continue;
        }
        if (rangereg) {
            put(rangereg);
            put(rangedigit);
            rangereg = 0;
        }
        if ((BYTE)linepos[-1] > 32 && linepos[-1] != ',' && linepos[-1] != '/'
            && linepos[-1] != '-')
            put('/');
        put(c);
        put(digit);
        last = code;
    }
    if (rangereg) {
        put(rangereg);
        put(rangedigit);
    }
}

/* DecodeSpecial: a special operand */
static void decodespecial(UWORD code, UWORD op)
{
    const UBYTE *at;
    ULONG n;
    UWORD x, y;

    switch (code) {
    case IMMED:
        immediate();
        return;
    case RRRMMM11:                      /* MOVE's destination */
        x = (op & 0xfc0) >> 6;
        decodeearev(x & 7, (x & 0x38) >> 3);
        return;
    case TOCCR:
        put('C');
        put('C');
        put('R');
        return;
    case TOSR:
        put('S');
        put('R');
        return;
    case DESTADDR11:
    case DESTDATA11:
        put(code == DESTADDR11 ? 'a' : 'd');
        put((char)('0' + ((op & 0xe00) >> 9)));
        return;
    case DESTDATA2:
    case DESTADDR2:
        put(code == DESTADDR2 ? 'a' : 'd');
        put((char)('0' + (op & 7)));
        return;
    case QIMMED11:
        x = (op & 0xe00) >> 9;
        put('#');
        put((char)('0' + (x ? x : 8)));
        return;
    case DISPLACE:                      /* Bcc: 8, 16 or 32 bit */
        at = instr;
        x = op & 0xff;
        if (!x)
            n = (ULONG)(LONG)(WORD)rdw();
        else if (x == 0xff)
            n = rdl();
        else
            n = (ULONG)(LONG)(BYTE)x;
        opstr = hexprint(n + (ULONG)at);
        strcopy();
        return;
    case DISPLACE2:                     /* DBcc */
        at = instr;
        n = (ULONG)(LONG)(WORD)rdw() + (ULONG)at;
        opstr = hexprint(n);
        strcopy();
        return;
    case IMMED2:
        put('#');
        put((char)('0' + (op & 7)));
        return;
    case DAREG215:
        x = v.extrainstr;
        *linepos = (x & 0x8000) ? 'a' : 'd';
        linepos++;
        put((char)('0' + ((x & 0x7000) >> 12)));
        return;
    case ADDRPI2:
    case ADDRPI11:
        x = code == ADDRPI2 ? (op & 7) : (op & 0xe00) >> 9;
        put('(');
        put('a');
        put((char)('0' + x));
        put(')');
        put('+');
        return;
    case ADDRPD2:
    case ADDRPD11:
        x = code == ADDRPD2 ? (op & 7) : (op & 0xe00) >> 9;
        put('-');
        put('(');
        put('a');
        put((char)('0' + x));
        put(')');
        return;
    case _DATA214202:                   /* MULx.l / DIVx.l: dl or dh:dl */
        x = (v.extrainstr & 0x7000) >> 12;
        y = v.extrainstr & 7;
        if (x != y) {
            put('d');
            put((char)('0' + y));
            put(':');
        }
        put('d');
        put((char)('0' + x));
        return;
    case TUSP:
        put('u');
        put('s');
        put('p');
        return;
    case CONTROL2: {
        static const struct { UWORD code; char name[5]; } ctrl[] = {
            { 0x000, "sfc" }, { 0x001, "dfc" }, { 0x002, "cacr" }, { 0x800, "usp" },
            { 0x801, "vbr" }, { 0x802, "caar" }, { 0x803, "msp" }, { 0x804, "isp" },
            { 0xffff, "???" } };
        const char *s;
        int i;

        x = v.extrainstr & 0xfff;
        for (i = 0; ctrl[i].code != 0xffff && ctrl[i].code != x; i++)
            ;
        for (s = ctrl[i].name; *s; s++)
            put(*s);
        return;
    }
    case RLIST:
        rlist(op);
        return;
    case BYTE7:                         /* MOVEQ */
        put('#');
        hexlast(op & 0xff, 2);
        return;
    case VNUM: {                        /* TRAP: "#" after HexPrint */
        char *p = hexprint(op & 0xf) + 6;

        put('#');
        *p = '$';
        opstr = p;
        strcopy();
        return;
    }
    }
    /* NONE and unknown codes: nothing */
}

/* an entry that matches: the line from column 0; 0 = illegal size, the
   search goes on with the next entry */
static int decode(const struct disdata *e, UWORD op)
{
    const char *n = e->name;            /* a3 */
    char c;
    int i;

    linepos = v.linebuff;
    for (i = 0; i < 9; i++)
        v.linebuff[i] = ' ';
    while ((c = *n++) != 0) {
        if (c == '*') {                 /* the condition of bits 8-11 */
            const char *cond = v.condtable;

            if (linepos[-1] == 'b' && linepos[-2] != 'd')
                cond = v.bcondtable;    /* Bcc: ra, sr ... */
            cond += ((op & 0xf00) >> 8) * 2;
            put(cond[0]);
            put(cond[1]);
            break;
        }
        put(c);
    }
    if (linepos[-3] == '!') {          /* DIVx.l/MULx.l: bit 11 of word 2 */
        linepos[-3] = 's';
        if (peekw() & 0x800)
            linepos[-3] = 'u';
    }
    v.opextra = 0;
    if (!e->szmask) {
        v.opsize = e->szshift & 0xff;   /* fixed, not printed */
    } else {
        UWORD m = e->szmask, s = lsrw(op & m, e->szshift & 0xff);

        if (m == 0b11)
            s--;
        if (m == 0b1000000)
            s++;
        if (m == 0b100000000)
            s++;
        if (m == 0b110000000 && s == 0b11)
            s = 0b01;
        if (linepos[-1] != '.')
            put('.');
        if (s == 0) {
            put('b');
            v.opsize = 0;
        } else if (s == 1) {
            put('w');
            v.opsize = 1;
        } else if (s == 2) {
            put('l');
            v.opsize = 2;
        } else {
            return 0;
        }
    }
    linepos = v.linebuff + 9;
    if (e->szshift & 0xf000) {
        v.opextra = (e->szshift & 0xf000) >> 12;
        v.extrainstr = rdw();
    }
    opstr = linepos;
    if (e->srcmask) {
        decodeea(lsrw(op & e->srcmask, e->src));
    } else {
        v.isdestop = 0;
        decodespecial(e->src, op);
    }
    v.docomma = linepos != opstr;       /* BUG(orig) (#615): opstr moves too */
    if (e->dstmask) {
        UWORD ea = lsrw(op & e->dstmask, e->dst);

        if (v.docomma)
            put(',');
        decodeea(ea);
    } else {
        if (v.docomma && e->dst != NONE)
            put(',');
        v.isdestop = 1;
        decodespecial(e->dst, op);
    }
    v.linebuff[34] = 0;
    return 1;
}

/**
 * @brief DisAssemble$(address.l): the instruction at address as a line of
 * 34 columns.
 *
 * The first word is matched against DisData; the line (name and size in
 * columns 0-8, the operands from column 9, spaces behind) is written to
 * a3 with 17 word writes.  InstrSize becomes the bytes the instruction
 * took.  BUG(orig): a decimal 0 is the bytes at address 8 (#614); a
 * source that only moves the operand pointer gets a comma (#615); longer
 * operands are cut after 34 columns, a line can overwrite the variables
 * behind LineBuff (#616); an odd a3 or address is an address error on a
 * 68000 (#617).
 * @param addr  d0: the address of the instruction
 * @return d0: 34, the length; 5 for an instruction not in the table
 *         ("?????", InstrSize 2)
 */
ULONG BLITZ_ROUTINE c_disoneline(ULONG addr BL_REG(d0))
{
    const struct disdata *e;
    UWORD op;
    int i;

    instr = (const UBYTE *)addr;
    disasm_add = addr;
    for (i = 0; i < 80; i++)
        v.linebuff[i] = ' ';
    op = rdw();
    for (e = disdata; ; e++) {
        if (!e->mask) {
            disasm_add = (ULONG)instr - disasm_add;
            for (i = 0; i < 5; i++)
                *blitz_string++ = '?';
            return 5;
        }
        if ((op & e->mask) == e->cmp && decode(e, op))
            break;
    }
    disasm_add = (ULONG)instr - disasm_add;
    for (i = 0; i < 17; i++) {          /* MOVE.w (a0)+,(a3)+ */
        *(volatile UWORD *)blitz_string = ((const UWORD *)v.linebuff)[i];
        blitz_string += 2;
    }
    return 34;
}

/**
 * @brief InstrSize: the size of the last instruction disassembled.
 * @return d0: its bytes (0 before the first DisAssemble$)
 */
ULONG BLITZ_ROUTINE c_instrsize(void)
{
    return disasm_add;
}

/**
 * @brief init: nothing (RTS), the library needs no system library.
 */
void BLITZ_ROUTINE libinit(void)
{
}

/**
 * @brief finit: nothing (RTS).
 */
void BLITZ_ROUTINE libfinit(void)
{
}

BLITZ_THUNK(disoneline, c_disoneline, "d1/a0-a1");
BLITZ_THUNK(instrsize, c_instrsize, "d1/a0-a1");
