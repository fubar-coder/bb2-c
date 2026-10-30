/*
 * main.c - chunk 01: blitz2.s lines 1-1911 (+ include wbheader, asmmac).
 *
 * Startup (main() replaces wbheader + begin), the token list, edstruct and
 * menus handed to the editor (blitz2:ted), the editor (comdata) wrappers,
 * the object-code templates (`objects`), the menu handlers, running the
 * compiled program in its own process, the libs directory scan and the
 * asmmac checker routines of the inline assembler.
 * See notes/survey-01.md.
 */
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <exec/execbase.h>
#include <dos/dosextens.h>
#include <workbench/startup.h>
#include "blitz2.h"

/* library bases: asm `maths` (1265) and `int` (14866, editor's base).
   asm `dos` (wbheader:107) is libnix's DOSBase. */
struct Library *MathBase;
struct IntuitionBase *IntuitionBase;

/* ================================================== wbheader data */

struct Task *us;                        /* wb:108 */
struct WBStartup *wbmess;               /* wb:109 */
char progfile[192];                     /* wb:110 */
static const char wb_infoname[] = ".info";      /* wb:103 */
/* wb:105 wb_dosname "dos.library": DIFF: libnix opens dos.library */

/* libnix: WBStartup message when started from Workbench */
extern struct WBStartup *_WBenchMsg;

/* ================================================== edstruct (274) */


/* ================================================== token list (283-722)
 *
 * One object holding all token nodes in source order, so the memory image
 * is the one of the asm (struct token: next.l, num.w, name\0, help\0,
 * even).  The asm mnemonic tokens (asmmac, fnum..fnum+117) are followed by
 * their struct asment data at the next even address (makeasmtab relies on
 * that).  `tokens` is an alias of the first node. */

/* asmmac equates (EA masks / size codes).  Renamed with a trailing `_`
   because `long`, `word`, `byte` ... clash with C / NDK names. */
#define ndn_    0xfffe
#define nan_    0xfffd
#define nian_   0xfffb
#define nanp_   0xfff7
#define npan_   0xffef
#define ndan_   0xffdf
#define ndanxi_ 0xffbf
#define nabsw_  0xff7f
#define nabsl_  0xfeff
#define ndpc_   0xfdff
#define ndpcxi_ 0xfbff
#define nimm_   0xf7ff
#define nccr_   0xefff
#define nsr_    0xdfff
#define nusp_   0xbfff
#define nmem_   0x7fff
#define nall_   0
#define dn_     1
#define an_     2
#define ian_    4
#define anp_    8
#define pan_    16
#define dan_    32
#define danxi_  64
#define absw_   128
#define absl_   256
#define dpc_    512
#define dpcxi_  1024
#define imm_    2048
#define ccr_    4096
#define sr_     8192
#define usp_    16384
#define mem_    32768
#define all_    0xfff
#define byte_   0xff0
#define word_   0xf1f
#define long_   0x2ff
#define sall_   0x210
#define nosz_   0xfff
#define byteword_ 0xf10
#define wordlong_ 0x10f

struct tokimage {
    struct { struct token *next; UWORD num; char s[sizeof("NEWTYPE" "\0" ".Typename")]; } tok1;  /* asm label: tokens */
    struct { struct token *next; UWORD num; char s[sizeof("End" "\0" "[If|While|Select|Statement|Function|SetInt|SetErr|NEWTYPE|MACRO]")]; } tok2;
    struct { struct token *next; UWORD num; char s[sizeof("Let" "\0" "Var=|Operator Expression")]; } tok3;
    struct { struct token *next; UWORD num; char s[sizeof("Dim" "\0" "Arrayname [List] (Dimension1[,Dimension2..])")]; } tok4;
    struct { struct token *next; UWORD num; char s[sizeof("Goto" "\0" "Program Label")]; } tok5;
    struct { struct token *next; UWORD num; char s[sizeof("Gosub" "\0" "Program Label")]; } tok6;
    struct { struct token *next; UWORD num; char s[sizeof("Return" "\0" "")]; } tok7;
    struct { struct token *next; UWORD num; char s[sizeof("Statement" "\0" "Procedurename{[Parameter1[,Parameter2...]]}")]; } tok8;
    struct { struct token *next; UWORD num; char s[sizeof("Function" "\0" "[.Type] Procedurename{[Parameter1[,Parameter2...]]}")]; } tok9;
    struct { struct token *next; UWORD num; char s[sizeof("MouseWait" "\0" "")]; } tok10;
    struct { struct token *next; UWORD num; char s[sizeof("If" "\0" "Expression [Then...]")]; } tok11;
    struct { struct token *next; UWORD num; char s[sizeof("While" "\0" "Expression")]; } tok12;
    struct { struct token *next; UWORD num; char s[sizeof("Macro" "\0" "Macroname")]; } tok13;
    struct { struct token *next; UWORD num; char s[sizeof("Select" "\0" "Expression")]; } tok14;
    struct { struct token *next; UWORD num; char s[sizeof("Case" "\0" "Expression")]; } tok15;
    struct { struct token *next; UWORD num; char s[sizeof("Default" "\0" "")]; } tok16;
    struct { struct token *next; UWORD num; char s[sizeof("AsmExit" "\0" "")]; } tok17;
    struct { struct token *next; UWORD num; char s[sizeof("XINCLUDE" "\0" "Filename")]; } tok18;
    struct { struct token *next; UWORD num; char s[sizeof("INCLUDE" "\0" "Filename")]; } tok19;
    struct { struct token *next; UWORD num; char s[sizeof("Jimi" "\0" "")]; } tok20;
    struct { struct token *next; UWORD num; char s[sizeof("SHARED" "\0" "Var[,Var...]")]; } tok21;
    struct { struct token *next; UWORD num; char s[sizeof("For" "\0" "Var=Expression1 To Expression2 [Step Expression3]")]; } tok22;
    struct { struct token *next; UWORD num; char s[sizeof("Next" "\0" "[Var[,Var...]]")]; } tok23;
    struct { struct token *next; UWORD num; char s[sizeof("To" "\0" "Expression2 [Step Expression3]")]; } tok24;
    struct { struct token *next; UWORD num; char s[sizeof("Step" "\0" "increment")]; } tok25;
    struct { struct token *next; UWORD num; char s[sizeof("Data" "\0" "[.Type] Item[,Item...]")]; } tok26;
    struct { struct token *next; UWORD num; char s[sizeof("Read" "\0" "Var[,Var...]")]; } tok27;
    struct { struct token *next; UWORD num; char s[sizeof("CNIF" "\0" "Constant Comparison Constant")]; } tok28;
    struct { struct token *next; UWORD num; char s[sizeof("CSIF" "\0" "\"String\" Comparison \"String\"")]; } tok29;
    struct { struct token *next; UWORD num; char s[sizeof("CELSE" "\0" "")]; } tok30;
    struct { struct token *next; UWORD num; char s[sizeof("CEND" "\0" "")]; } tok31;
    struct { struct token *next; UWORD num; char s[sizeof("CERR" "\0" "Errormessage")]; } tok32;
    struct { struct token *next; UWORD num; char s[sizeof("Then" "\0" "statement...")]; } tok33;
    struct { struct token *next; UWORD num; char s[sizeof("Else" "\0" "[Statement...]")]; } tok34;
    struct { struct token *next; UWORD num; char s[sizeof("Even4" "\0" "; align to 4 byte boundary")]; } tok35;
    struct { struct token *next; UWORD num; char s[sizeof("Even8" "\0" "; align to 8 byte boundary")]; } tok36;
    struct { struct token *next; UWORD num; char s[sizeof("Dc" "\0" "[.Size] Data[,Data...]")]; } tok37;
    struct { struct token *next; UWORD num; char s[sizeof("Ds" "\0" "[.Size] Length")]; } tok38;
    struct { struct token *next; UWORD num; char s[sizeof("Even" "\0" "")]; } tok39;
    struct { struct token *next; UWORD num; char s[sizeof("Dcb" "\0" "[.Size] Repeats,Data")]; } tok40;
    struct { struct token *next; UWORD num; char s[sizeof("GetReg" "\0" "68000 Reg,Expression")]; } tok41;
    struct { struct token *next; UWORD num; char s[sizeof("IncBin" "\0" "Filename")]; } tok42;
    struct { struct token *next; UWORD num; char s[sizeof("Free" "\0" "Objectname Object#")]; } tok43;
    struct { struct token *next; UWORD num; char s[sizeof("Use" "\0" "Objectname Object#")]; } tok44;
    struct { struct token *next; UWORD num; char s[sizeof("Stop" "\0" "")]; } tok45;
    struct { struct token *next; UWORD num; char s[sizeof("Cont" "\0" "[N]")]; } tok46;
    struct { struct token *next; UWORD num; char s[sizeof("SizeOf" "\0" ".Typename[,Entrypath]")]; } tok47;
    struct { struct token *next; UWORD num; char s[sizeof("SetInt" "\0" "Type")]; } tok48;
    struct { struct token *next; UWORD num; char s[sizeof("ClrInt" "\0" "Type")]; } tok49;
    struct { struct token *next; UWORD num; char s[sizeof("MaxLen" "\0" "StringVar=Expression")]; } tok50;
    struct { struct token *next; UWORD num; char s[sizeof("DEFTYPE" "\0" ".Typename [Var[,Var...]]")]; } tok51;
    struct { struct token *next; UWORD num; char s[sizeof("BLITZ" "\0" "")]; } tok52;
    struct { struct token *next; UWORD num; char s[sizeof("AMIGA" "\0" "")]; } tok53;
    struct { struct token *next; UWORD num; char s[sizeof("QAMIGA" "\0" "")]; } tok54;
    struct { struct token *next; UWORD num; char s[sizeof("VWait" "\0" "[Frames]")]; } tok55;
    struct { struct token *next; UWORD num; char s[sizeof("ALibJsr" "\0" "Token[,Form]")]; } tok56;
    struct { struct token *next; UWORD num; char s[sizeof("TokeJsr" "\0" "Token[,Form]")]; } tok57;
    struct { struct token *next; UWORD num; char s[sizeof("BLibJsr" "\0" "Token[,Form]")]; } tok58;
    struct { struct token *next; UWORD num; char s[sizeof("Jimi" "\0" "")]; } tok59;
    struct { struct token *next; UWORD num; char s[sizeof("ABCD" "\0" "")]; struct asment d; } opabcd;
    struct { struct token *next; UWORD num; char s[sizeof("ADD" "\0" "")]; struct asment d; } opadd;
    struct { struct token *next; UWORD num; char s[sizeof("ADDI" "\0" "")]; struct asment d; } opaddi;
    struct { struct token *next; UWORD num; char s[sizeof("ADDA" "\0" "")]; struct asment d; } opadda;
    struct { struct token *next; UWORD num; char s[sizeof("ADDQ" "\0" "")]; struct asment d; } opaddq;
    struct { struct token *next; UWORD num; char s[sizeof("ADDX" "\0" "")]; struct asment d; } opaddx;
    struct { struct token *next; UWORD num; char s[sizeof("AND" "\0" "")]; struct asment d; } opand;
    struct { struct token *next; UWORD num; char s[sizeof("ANDI" "\0" "")]; struct asment d; } opandi;
    struct { struct token *next; UWORD num; char s[sizeof("ASL" "\0" "")]; struct asment d; } opasl;
    struct { struct token *next; UWORD num; char s[sizeof("ASR" "\0" "")]; struct asment d; } opasr;
    struct { struct token *next; UWORD num; char s[sizeof("BCC" "\0" "")]; struct asment d; } opbcc;
    struct { struct token *next; UWORD num; char s[sizeof("BCS" "\0" "")]; struct asment d; } opbcs;
    struct { struct token *next; UWORD num; char s[sizeof("BEQ" "\0" "")]; struct asment d; } opbeq;
    struct { struct token *next; UWORD num; char s[sizeof("BGE" "\0" "")]; struct asment d; } opbge;
    struct { struct token *next; UWORD num; char s[sizeof("BGT" "\0" "")]; struct asment d; } opbgt;
    struct { struct token *next; UWORD num; char s[sizeof("BHI" "\0" "")]; struct asment d; } opbhi;
    struct { struct token *next; UWORD num; char s[sizeof("BLE" "\0" "")]; struct asment d; } opble;
    struct { struct token *next; UWORD num; char s[sizeof("BLS" "\0" "")]; struct asment d; } opbls;
    struct { struct token *next; UWORD num; char s[sizeof("BLT" "\0" "")]; struct asment d; } opblt;
    struct { struct token *next; UWORD num; char s[sizeof("BMI" "\0" "")]; struct asment d; } opbmi;
    struct { struct token *next; UWORD num; char s[sizeof("BNE" "\0" "")]; struct asment d; } opbne;
    struct { struct token *next; UWORD num; char s[sizeof("BPL" "\0" "")]; struct asment d; } opbpl;
    struct { struct token *next; UWORD num; char s[sizeof("BVC" "\0" "")]; struct asment d; } opbvc;
    struct { struct token *next; UWORD num; char s[sizeof("BVS" "\0" "")]; struct asment d; } opbvs;
    struct { struct token *next; UWORD num; char s[sizeof("BCHG" "\0" "")]; struct asment d; } opbchg;
    struct { struct token *next; UWORD num; char s[sizeof("BCLR" "\0" "")]; struct asment d; } opbclr;
    struct { struct token *next; UWORD num; char s[sizeof("BSET" "\0" "")]; struct asment d; } opbset;
    struct { struct token *next; UWORD num; char s[sizeof("BTST" "\0" "")]; struct asment d; } opbtst;
    struct { struct token *next; UWORD num; char s[sizeof("BRA" "\0" "")]; struct asment d; } opbra;
    struct { struct token *next; UWORD num; char s[sizeof("BSR" "\0" "")]; struct asment d; } opbsr;
    struct { struct token *next; UWORD num; char s[sizeof("CHK" "\0" "")]; struct asment d; } opchk;
    struct { struct token *next; UWORD num; char s[sizeof("CLR" "\0" "")]; struct asment d; } opclr;
    struct { struct token *next; UWORD num; char s[sizeof("CMP" "\0" "")]; struct asment d; } opcmp;
    struct { struct token *next; UWORD num; char s[sizeof("CMPI" "\0" "")]; struct asment d; } opcmpi;
    struct { struct token *next; UWORD num; char s[sizeof("CMPA" "\0" "")]; struct asment d; } opcmpa;
    struct { struct token *next; UWORD num; char s[sizeof("CMPM" "\0" "")]; struct asment d; } opcmpm;
    struct { struct token *next; UWORD num; char s[sizeof("DBCC" "\0" "")]; struct asment d; } opdbcc;
    struct { struct token *next; UWORD num; char s[sizeof("DBCS" "\0" "")]; struct asment d; } opdbcs;
    struct { struct token *next; UWORD num; char s[sizeof("DBEQ" "\0" "")]; struct asment d; } opdbeq;
    struct { struct token *next; UWORD num; char s[sizeof("DBGE" "\0" "")]; struct asment d; } opdbge;
    struct { struct token *next; UWORD num; char s[sizeof("DBGT" "\0" "")]; struct asment d; } opdbgt;
    struct { struct token *next; UWORD num; char s[sizeof("DBHI" "\0" "")]; struct asment d; } opdbhi;
    struct { struct token *next; UWORD num; char s[sizeof("DBLE" "\0" "")]; struct asment d; } opdble;
    struct { struct token *next; UWORD num; char s[sizeof("DBLS" "\0" "")]; struct asment d; } opdbls;
    struct { struct token *next; UWORD num; char s[sizeof("DBLT" "\0" "")]; struct asment d; } opdblt;
    struct { struct token *next; UWORD num; char s[sizeof("DBMI" "\0" "")]; struct asment d; } opdbmi;
    struct { struct token *next; UWORD num; char s[sizeof("DBNE" "\0" "")]; struct asment d; } opdbne;
    struct { struct token *next; UWORD num; char s[sizeof("DBPL" "\0" "")]; struct asment d; } opdbpl;
    struct { struct token *next; UWORD num; char s[sizeof("DBVC" "\0" "")]; struct asment d; } opdbvc;
    struct { struct token *next; UWORD num; char s[sizeof("DBVS" "\0" "")]; struct asment d; } opdbvs;
    struct { struct token *next; UWORD num; char s[sizeof("DBF" "\0" "")]; struct asment d; } opdbf;
    struct { struct token *next; UWORD num; char s[sizeof("DBRA" "\0" "")]; struct asment d; } opdra;
    struct { struct token *next; UWORD num; char s[sizeof("DBT" "\0" "")]; struct asment d; } opdbt;
    struct { struct token *next; UWORD num; char s[sizeof("DIVS" "\0" "")]; struct asment d; } opdivs;
    struct { struct token *next; UWORD num; char s[sizeof("DIVU" "\0" "")]; struct asment d; } opdivu;
    struct { struct token *next; UWORD num; char s[sizeof("EOR" "\0" "")]; struct asment d; } opeor;
    struct { struct token *next; UWORD num; char s[sizeof("EORI" "\0" "")]; struct asment d; } opeori;
    struct { struct token *next; UWORD num; char s[sizeof("EXG" "\0" "")]; struct asment d; } opexg;
    struct { struct token *next; UWORD num; char s[sizeof("EXT" "\0" "")]; struct asment d; } opext;
    struct { struct token *next; UWORD num; char s[sizeof("ILLEGAL" "\0" "")]; struct asment d; } opillegal;
    struct { struct token *next; UWORD num; char s[sizeof("JMP" "\0" "")]; struct asment d; } opjmp;
    struct { struct token *next; UWORD num; char s[sizeof("JSR" "\0" "")]; struct asment d; } opjsr;
    struct { struct token *next; UWORD num; char s[sizeof("LEA" "\0" "")]; struct asment d; } oplea;
    struct { struct token *next; UWORD num; char s[sizeof("LINK" "\0" "")]; struct asment d; } oplink;
    struct { struct token *next; UWORD num; char s[sizeof("LSL" "\0" "")]; struct asment d; } oplsl;
    struct { struct token *next; UWORD num; char s[sizeof("LSR" "\0" "")]; struct asment d; } oplsr;
    struct { struct token *next; UWORD num; char s[sizeof("MOVE" "\0" "")]; struct asment d; } opmove;
    struct { struct token *next; UWORD num; char s[sizeof("MOVEP" "\0" "")]; struct asment d; } opmovep;
    struct { struct token *next; UWORD num; char s[sizeof("MOVEA" "\0" "")]; struct asment d; } opmovea;
    struct { struct token *next; UWORD num; char s[sizeof("MOVEQ" "\0" "")]; struct asment d; } opmoveq;
    struct { struct token *next; UWORD num; char s[sizeof("MOVEM" "\0" "")]; struct asment d; } opmovem;
    struct { struct token *next; UWORD num; char s[sizeof("MULS" "\0" "")]; struct asment d; } opmuls;
    struct { struct token *next; UWORD num; char s[sizeof("MULU" "\0" "")]; struct asment d; } opmulu;
    struct { struct token *next; UWORD num; char s[sizeof("NBCD" "\0" "")]; struct asment d; } opnbcd;
    struct { struct token *next; UWORD num; char s[sizeof("NEG" "\0" "")]; struct asment d; } opneg;
    struct { struct token *next; UWORD num; char s[sizeof("NEGX" "\0" "")]; struct asment d; } opnegx;
    struct { struct token *next; UWORD num; char s[sizeof("NOP" "\0" "")]; struct asment d; } opnop;
    struct { struct token *next; UWORD num; char s[sizeof("NOT" "\0" "")]; struct asment d; } opnot;
    struct { struct token *next; UWORD num; char s[sizeof("OR" "\0" "")]; struct asment d; } opor;
    struct { struct token *next; UWORD num; char s[sizeof("ORI" "\0" "")]; struct asment d; } opori;
    struct { struct token *next; UWORD num; char s[sizeof("PEA" "\0" "")]; struct asment d; } oppea;
    struct { struct token *next; UWORD num; char s[sizeof("RESET" "\0" "")]; struct asment d; } opreset;
    struct { struct token *next; UWORD num; char s[sizeof("ROL" "\0" "")]; struct asment d; } oprol;
    struct { struct token *next; UWORD num; char s[sizeof("ROR" "\0" "")]; struct asment d; } opror;
    struct { struct token *next; UWORD num; char s[sizeof("ROXL" "\0" "")]; struct asment d; } oproxl;
    struct { struct token *next; UWORD num; char s[sizeof("ROXR" "\0" "")]; struct asment d; } oproxr;
    struct { struct token *next; UWORD num; char s[sizeof("RTE" "\0" "")]; struct asment d; } oprte;
    struct { struct token *next; UWORD num; char s[sizeof("RTR" "\0" "")]; struct asment d; } oprtr;
    struct { struct token *next; UWORD num; char s[sizeof("RTS" "\0" "")]; struct asment d; } oprts;
    struct { struct token *next; UWORD num; char s[sizeof("SBCD" "\0" "")]; struct asment d; } opsbcd;
    struct { struct token *next; UWORD num; char s[sizeof("SCC" "\0" "")]; struct asment d; } opscc;
    struct { struct token *next; UWORD num; char s[sizeof("SCS" "\0" "")]; struct asment d; } opscs;
    struct { struct token *next; UWORD num; char s[sizeof("SEQ" "\0" "")]; struct asment d; } opseq;
    struct { struct token *next; UWORD num; char s[sizeof("SGE" "\0" "")]; struct asment d; } opsge;
    struct { struct token *next; UWORD num; char s[sizeof("SGT" "\0" "")]; struct asment d; } opsgt;
    struct { struct token *next; UWORD num; char s[sizeof("SHI" "\0" "")]; struct asment d; } opshi;
    struct { struct token *next; UWORD num; char s[sizeof("SLE" "\0" "")]; struct asment d; } opsle;
    struct { struct token *next; UWORD num; char s[sizeof("SLS" "\0" "")]; struct asment d; } opsls;
    struct { struct token *next; UWORD num; char s[sizeof("SLT" "\0" "")]; struct asment d; } opslt;
    struct { struct token *next; UWORD num; char s[sizeof("SMI" "\0" "")]; struct asment d; } opsmi;
    struct { struct token *next; UWORD num; char s[sizeof("SNE" "\0" "")]; struct asment d; } opsne;
    struct { struct token *next; UWORD num; char s[sizeof("SPL" "\0" "")]; struct asment d; } opspl;
    struct { struct token *next; UWORD num; char s[sizeof("SVC" "\0" "")]; struct asment d; } opsvc;
    struct { struct token *next; UWORD num; char s[sizeof("SVS" "\0" "")]; struct asment d; } opsvs;
    struct { struct token *next; UWORD num; char s[sizeof("ST" "\0" "")]; struct asment d; } opst;
    struct { struct token *next; UWORD num; char s[sizeof("SF" "\0" "")]; struct asment d; } opsf;
    struct { struct token *next; UWORD num; char s[sizeof("STOP" "\0" "")]; struct asment d; } opstop;
    struct { struct token *next; UWORD num; char s[sizeof("SUB" "\0" "")]; struct asment d; } opsub;
    struct { struct token *next; UWORD num; char s[sizeof("SUBI" "\0" "")]; struct asment d; } opsubi;
    struct { struct token *next; UWORD num; char s[sizeof("SUBA" "\0" "")]; struct asment d; } opsuba;
    struct { struct token *next; UWORD num; char s[sizeof("SUBQ" "\0" "")]; struct asment d; } opsubq;
    struct { struct token *next; UWORD num; char s[sizeof("SUBX" "\0" "")]; struct asment d; } opsubx;
    struct { struct token *next; UWORD num; char s[sizeof("SWAP" "\0" "")]; struct asment d; } opswap;
    struct { struct token *next; UWORD num; char s[sizeof("TAS" "\0" "")]; struct asment d; } optas;
    struct { struct token *next; UWORD num; char s[sizeof("TRAP" "\0" "")]; struct asment d; } optrap;
    struct { struct token *next; UWORD num; char s[sizeof("TRAPV" "\0" "")]; struct asment d; } optrapv;
    struct { struct token *next; UWORD num; char s[sizeof("TST" "\0" "")]; struct asment d; } optst;
    struct { struct token *next; UWORD num; char s[sizeof("UNLK" "\0" "")]; struct asment d; } opunlk;
    struct { struct token *next; UWORD num; char s[sizeof("EndAsm" "\0" "")]; } endasm;
    struct { struct token *next; UWORD num; char s[sizeof("List" "\0" "")]; } toktnum;
    struct { struct token *next; UWORD num; char s[sizeof("SetErr" "\0" "")]; } toktnum2;
    struct { struct token *next; UWORD num; char s[sizeof("ClrErr" "\0" "")]; } toktnum3;
    struct { struct token *next; UWORD num; char s[sizeof("ErrFail" "\0" "")]; } toktnum5;
    struct { struct token *next; UWORD num; char s[sizeof("Addr" "\0" "Objectname(Object#)")]; } toktnum6;
    struct { struct token *next; UWORD num; char s[sizeof("MOD" "\0" "")]; } toktnum7;
    struct { struct token *next; UWORD num; char s[sizeof("Pi" "\0" "")]; } toktnum8;
    struct { struct token *next; UWORD num; char s[sizeof("Repeat" "\0" "")]; } toktnum9;
    struct { struct token *next; UWORD num; char s[sizeof("Until" "\0" "Expression")]; } toktnum10;
    struct { struct token *next; UWORD num; char s[sizeof("PutReg" "\0" "68000 Reg,Variable")]; } toktnum11;
    struct { struct token *next; UWORD num; char s[sizeof("Pop" "\0" "Gosub|For|Select|If|While|Repeat")]; } toktnum12;
    struct { struct token *next; UWORD num; char s[sizeof("INCDIR" "\0" "Pathname")]; } toktnum13;
    struct { struct token *next; UWORD num; char s[sizeof("EndIf" "\0" "")]; } toktnum14;
    struct { struct token *next; UWORD num; char s[sizeof("Wend" "\0" "")]; } toktnum15;
    struct { struct token *next; UWORD num; char s[sizeof("SysJsr" "\0" "Routine")]; } toktnum16;
    struct { struct token *next; UWORD num; char s[sizeof("WBStartup" "\0" "")]; } toktnum17;
    struct { struct token *next; UWORD num; char s[sizeof("Maximum" "\0" "Objectname")]; } toktnum18;
    struct { struct token *next; UWORD num; char s[sizeof("On" "\0" "Expression Goto|Gosub Program Label[,Program Label...]")]; } toktnum19;
    struct { struct token *next; UWORD num; char s[sizeof("Off" "\0" "")]; } toktnum20;
    struct { struct token *next; UWORD num; char s[sizeof("Forever" "\0" "")]; } toktnum21;
    struct { struct token *next; UWORD num; char s[sizeof("Restore" "\0" "[Program Label]")]; } toktnum22;
    struct { struct token *next; UWORD num; char s[sizeof("Exchange" "\0" "Var,Var")]; } toktnum23;
    struct { struct token *next; UWORD num; char s[sizeof("USEPATH" "\0" "Pathtext")]; } toktnum24;
    struct { struct token *next; UWORD num; char s[sizeof("CloseEd" "\0" "")]; } toktnum25;
    struct { struct token *next; UWORD num; char s[sizeof("NoCli" "\0" "")]; } toktnum26;
    struct { struct token *next; UWORD num; char s[sizeof("BitTst" "\0" "")]; } toktnum27;
    struct { struct token *next; UWORD num; char s[sizeof("BitSet" "\0" "")]; } toktnum28;
    struct { struct token *next; UWORD num; char s[sizeof("BitClr" "\0" "")]; } toktnum29;
    struct { struct token *next; UWORD num; char s[sizeof("BitChg" "\0" "")]; } toktnum30;
    struct { struct token *next; UWORD num; char s[sizeof("Used" "\0" "Objectname(Object#)")]; } toktnum31;
    struct { struct token *next; UWORD num; char s[sizeof("Runerrson" "\0" "")]; } toktnum32;
    struct { struct token *next; UWORD num; char s[sizeof("Runerrsoff" "\0" "")]; } toktnum33;
} tokimg = {
    { (struct token *)&tokimg.tok2, 1, "NEWTYPE" "\0" ".Typename" },
    { (struct token *)&tokimg.tok3, 2, "End" "\0" "[If|While|Select|Statement|Function|SetInt|SetErr|NEWTYPE|MACRO]" },
    { (struct token *)&tokimg.tok4, 3, "Let" "\0" "Var=|Operator Expression" },
    { (struct token *)&tokimg.tok5, 4, "Dim" "\0" "Arrayname [List] (Dimension1[,Dimension2..])" },
    { (struct token *)&tokimg.tok6, 5, "Goto" "\0" "Program Label" },
    { (struct token *)&tokimg.tok7, 6, "Gosub" "\0" "Program Label" },
    { (struct token *)&tokimg.tok8, 7, "Return" "\0" "" },
    { (struct token *)&tokimg.tok9, 8, "Statement" "\0" "Procedurename{[Parameter1[,Parameter2...]]}" },
    { (struct token *)&tokimg.tok10, 9, "Function" "\0" "[.Type] Procedurename{[Parameter1[,Parameter2...]]}" },
    { (struct token *)&tokimg.tok11, 10, "MouseWait" "\0" "" },
    { (struct token *)&tokimg.tok12, 11, "If" "\0" "Expression [Then...]" },
    { (struct token *)&tokimg.tok13, 12, "While" "\0" "Expression" },
    { (struct token *)&tokimg.tok14, 13, "Macro" "\0" "Macroname" },
    { (struct token *)&tokimg.tok15, 14, "Select" "\0" "Expression" },
    { (struct token *)&tokimg.tok16, 15, "Case" "\0" "Expression" },
    { (struct token *)&tokimg.tok17, 16, "Default" "\0" "" },
    { (struct token *)&tokimg.tok18, 17, "AsmExit" "\0" "" },
    { (struct token *)&tokimg.tok19, 18, "XINCLUDE" "\0" "Filename" },
    { (struct token *)&tokimg.tok20, 19, "INCLUDE" "\0" "Filename" },
    { (struct token *)&tokimg.tok21, 20, "Jimi" "\0" "" },
    { (struct token *)&tokimg.tok22, 21, "SHARED" "\0" "Var[,Var...]" },
    { (struct token *)&tokimg.tok23, 22, "For" "\0" "Var=Expression1 To Expression2 [Step Expression3]" },
    { (struct token *)&tokimg.tok24, 23, "Next" "\0" "[Var[,Var...]]" },
    { (struct token *)&tokimg.tok25, 24, "To" "\0" "Expression2 [Step Expression3]" },
    { (struct token *)&tokimg.tok26, 25, "Step" "\0" "increment" },
    { (struct token *)&tokimg.tok27, 26, "Data" "\0" "[.Type] Item[,Item...]" },
    { (struct token *)&tokimg.tok28, 27, "Read" "\0" "Var[,Var...]" },
    { (struct token *)&tokimg.tok29, 28, "CNIF" "\0" "Constant Comparison Constant" },
    { (struct token *)&tokimg.tok30, 29, "CSIF" "\0" "\"String\" Comparison \"String\"" },
    { (struct token *)&tokimg.tok31, 30, "CELSE" "\0" "" },
    { (struct token *)&tokimg.tok32, 31, "CEND" "\0" "" },
    { (struct token *)&tokimg.tok33, 32, "CERR" "\0" "Errormessage" },
    { (struct token *)&tokimg.tok34, 33, "Then" "\0" "statement..." },
    { (struct token *)&tokimg.tok35, 34, "Else" "\0" "[Statement...]" },
    { (struct token *)&tokimg.tok36, 35, "Even4" "\0" "; align to 4 byte boundary" },
    { (struct token *)&tokimg.tok37, 36, "Even8" "\0" "; align to 8 byte boundary" },
    { (struct token *)&tokimg.tok38, 37, "Dc" "\0" "[.Size] Data[,Data...]" },
    { (struct token *)&tokimg.tok39, 38, "Ds" "\0" "[.Size] Length" },
    { (struct token *)&tokimg.tok40, 39, "Even" "\0" "" },
    { (struct token *)&tokimg.tok41, 40, "Dcb" "\0" "[.Size] Repeats,Data" },
    { (struct token *)&tokimg.tok42, 41, "GetReg" "\0" "68000 Reg,Expression" },
    { (struct token *)&tokimg.tok43, 42, "IncBin" "\0" "Filename" },
    { (struct token *)&tokimg.tok44, 43, "Free" "\0" "Objectname Object#" },
    { (struct token *)&tokimg.tok45, 44, "Use" "\0" "Objectname Object#" },
    { (struct token *)&tokimg.tok46, 45, "Stop" "\0" "" },
    { (struct token *)&tokimg.tok47, 46, "Cont" "\0" "[N]" },
    { (struct token *)&tokimg.tok48, 47, "SizeOf" "\0" ".Typename[,Entrypath]" },
    { (struct token *)&tokimg.tok49, 48, "SetInt" "\0" "Type" },
    { (struct token *)&tokimg.tok50, 49, "ClrInt" "\0" "Type" },
    { (struct token *)&tokimg.tok51, 50, "MaxLen" "\0" "StringVar=Expression" },
    { (struct token *)&tokimg.tok52, 51, "DEFTYPE" "\0" ".Typename [Var[,Var...]]" },
    { (struct token *)&tokimg.tok53, 52, "BLITZ" "\0" "" },
    { (struct token *)&tokimg.tok54, 53, "AMIGA" "\0" "" },
    { (struct token *)&tokimg.tok55, 54, "QAMIGA" "\0" "" },
    { (struct token *)&tokimg.tok56, 55, "VWait" "\0" "[Frames]" },
    { (struct token *)&tokimg.tok57, 56, "ALibJsr" "\0" "Token[,Form]" },
    { (struct token *)&tokimg.tok58, 57, "TokeJsr" "\0" "Token[,Form]" },
    { (struct token *)&tokimg.tok59, 58, "BLibJsr" "\0" "Token[,Form]" },
    { (struct token *)&tokimg.opabcd, 59, "Jimi" "\0" "" },
    { (struct token *)&tokimg.opadd, fnum+0, "ABCD" "\0" "",
      { 0xc100, dn_^pan_, dn_^pan_, (9<<4)+0, (0<<4)+0, (UWORD)((15<<12)+byte_), abcdchk, (WORD)0 } },
    { (struct token *)&tokimg.opaddi, fnum+1, "ADD" "\0" "",
      { 0xd000, all_, 0x1fd, (0<<4)+3, (9<<4)+15, (UWORD)((6<<12)+sall_), addchk, (WORD)0x8003 } },
    { (struct token *)&tokimg.opadda, fnum+2, "ADDI" "\0" "",
      { 0x600, imm_, 0x1fd, (15<<4)+15, (0<<4)+3, (UWORD)((6<<12)+sall_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opaddq, fnum+3, "ADDA" "\0" "",
      { 0xd0c0, all_, 2, (0<<4)+3, (9<<4)+15, (UWORD)((8<<12)+wordlong_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opaddx, fnum+4, "ADDQ" "\0" "",
      { 0x5000, imm_, 0x1ff, (15<<4)+15, (0<<4)+3, (UWORD)((6<<12)+sall_), NULL, (WORD)1 } },
    { (struct token *)&tokimg.opand, fnum+5, "ADDX" "\0" "",
      { 0xd100, dn_^pan_, dn_^pan_, (0<<4)+15, (9<<4)+15, (UWORD)((6<<12)+sall_), abcdchk, (WORD)0 } },
    { (struct token *)&tokimg.opandi, fnum+6, "AND" "\0" "",
      { 0xc000, all_&nan_, 0x1fd, (0<<4)+3, (9<<4)+15, (UWORD)((6<<12)+sall_), addchk, (WORD)0 } },
    { (struct token *)&tokimg.opasl, fnum+7, "ANDI" "\0" "",
      { 0x200, imm_, 0x1fd^ccr_^sr_^imm_, (15<<4)+15, (0<<4)+3, (UWORD)((6<<12)+sall_), andichk, (WORD)0 } },
    { (struct token *)&tokimg.opasr, fnum+8, "ASL" "\0" "",
      { 0xe100, imm_^0x1fd, 1, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+sall_), aslchk, (WORD)1 } },
    { (struct token *)&tokimg.opbcc, fnum+9, "ASR" "\0" "",
      { 0xe000, imm_^0x1fd, 1, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+sall_), aslchk, (WORD)1 } },
    { (struct token *)&tokimg.opbcs, fnum+10, "BCC" "\0" "",
      { 0x6400, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2 } },
    { (struct token *)&tokimg.opbeq, fnum+11, "BCS" "\0" "",
      { 0x6500, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2 } },
    { (struct token *)&tokimg.opbge, fnum+12, "BEQ" "\0" "",
      { 0x6700, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2 } },
    { (struct token *)&tokimg.opbgt, fnum+13, "BGE" "\0" "",
      { 0x6C00, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2 } },
    { (struct token *)&tokimg.opbhi, fnum+14, "BGT" "\0" "",
      { 0x6E00, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2 } },
    { (struct token *)&tokimg.opble, fnum+15, "BHI" "\0" "",
      { 0x6200, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2 } },
    { (struct token *)&tokimg.opbls, fnum+16, "BLE" "\0" "",
      { 0x6F00, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2 } },
    { (struct token *)&tokimg.opblt, fnum+17, "BLS" "\0" "",
      { 0x6300, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2 } },
    { (struct token *)&tokimg.opbmi, fnum+18, "BLT" "\0" "",
      { 0x6D00, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2 } },
    { (struct token *)&tokimg.opbne, fnum+19, "BMI" "\0" "",
      { 0x6B00, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2 } },
    { (struct token *)&tokimg.opbpl, fnum+20, "BNE" "\0" "",
      { 0x6600, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2 } },
    { (struct token *)&tokimg.opbvc, fnum+21, "BPL" "\0" "",
      { 0x6A00, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2 } },
    { (struct token *)&tokimg.opbvs, fnum+22, "BVC" "\0" "",
      { 0x6800, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2 } },
    { (struct token *)&tokimg.opbchg, fnum+23, "BVS" "\0" "",
      { 0x6900, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2 } },
    { (struct token *)&tokimg.opbclr, fnum+24, "BCHG" "\0" "",
      { 0x140, imm_^dn_, 0x1fd, (15<<4)+15, (0<<4)+3, (UWORD)((15<<12)+sall_), bitchk, (WORD)0 } },
    { (struct token *)&tokimg.opbset, fnum+25, "BCLR" "\0" "",
      { 0x180, imm_^dn_, 0x1fd, (15<<4)+15, (0<<4)+3, (UWORD)((15<<12)+sall_), bitchk, (WORD)0 } },
    { (struct token *)&tokimg.opbtst, fnum+26, "BSET" "\0" "",
      { 0x1c0, imm_^dn_, 0x1fd, (15<<4)+15, (0<<4)+3, (UWORD)((15<<12)+sall_), bitchk, (WORD)0 } },
    { (struct token *)&tokimg.opbra, fnum+27, "BTST" "\0" "",
      { 0x100, imm_^dn_, 0x1fd, (15<<4)+15, (0<<4)+3, (UWORD)((15<<12)+sall_), bitchk, (WORD)0 } },
    { (struct token *)&tokimg.opbsr, fnum+28, "BRA" "\0" "",
      { 0x6000, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)2 } },
    { (struct token *)&tokimg.opchk, fnum+29, "BSR" "\0" "",
      { 0x6100, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)2 } },
    { (struct token *)&tokimg.opclr, fnum+30, "CHK" "\0" "",
      { 0x4180, 0xffd, dn_, (0<<4)+3, (9<<4)+15, (UWORD)((15<<12)+word_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opcmp, fnum+31, "CLR" "\0" "",
      { 0x4200, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((6<<12)+sall_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opcmpi, fnum+32, "CMP" "\0" "",
      { 0xb000, all_, dn_, (0<<4)+3, (9<<4)+15, (UWORD)((6<<12)+sall_), NULL, (WORD)0x8003 } },
    { (struct token *)&tokimg.opcmpa, fnum+33, "CMPI" "\0" "",
      { 0xc00, imm_, 0x1fd, (15<<4)+15, (0<<4)+3, (UWORD)((6<<12)+sall_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opcmpm, fnum+34, "CMPA" "\0" "",
      { 0xb0c0, all_, an_, (0<<4)+3, (9<<4)+15, (UWORD)((8<<12)+wordlong_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opdbcc, fnum+35, "CMPM" "\0" "",
      { 0xb108, anp_, anp_, (0<<4)+15, (9<<4)+15, (UWORD)((6<<12)+sall_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opdbcs, fnum+36, "DBCC" "\0" "",
      { 0x54C8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5 } },
    { (struct token *)&tokimg.opdbeq, fnum+37, "DBCS" "\0" "",
      { 0x55C8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5 } },
    { (struct token *)&tokimg.opdbge, fnum+38, "DBEQ" "\0" "",
      { 0x57C8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5 } },
    { (struct token *)&tokimg.opdbgt, fnum+39, "DBGE" "\0" "",
      { 0x5CC8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5 } },
    { (struct token *)&tokimg.opdbhi, fnum+40, "DBGT" "\0" "",
      { 0x5EC8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5 } },
    { (struct token *)&tokimg.opdble, fnum+41, "DBHI" "\0" "",
      { 0x52C8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5 } },
    { (struct token *)&tokimg.opdbls, fnum+42, "DBLE" "\0" "",
      { 0x5FC8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5 } },
    { (struct token *)&tokimg.opdblt, fnum+43, "DBLS" "\0" "",
      { 0x53C8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5 } },
    { (struct token *)&tokimg.opdbmi, fnum+44, "DBLT" "\0" "",
      { 0x5DC8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5 } },
    { (struct token *)&tokimg.opdbne, fnum+45, "DBMI" "\0" "",
      { 0x5BC8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5 } },
    { (struct token *)&tokimg.opdbpl, fnum+46, "DBNE" "\0" "",
      { 0x56C8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5 } },
    { (struct token *)&tokimg.opdbvc, fnum+47, "DBPL" "\0" "",
      { 0x5AC8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5 } },
    { (struct token *)&tokimg.opdbvs, fnum+48, "DBVC" "\0" "",
      { 0x58C8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5 } },
    { (struct token *)&tokimg.opdbf, fnum+49, "DBVS" "\0" "",
      { 0x59C8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5 } },
    { (struct token *)&tokimg.opdra, fnum+50, "DBF" "\0" "",
      { 0x51c8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5 } },
    { (struct token *)&tokimg.opdbt, fnum+51, "DBRA" "\0" "",
      { 0x51c8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5 } },
    { (struct token *)&tokimg.opdivs, fnum+52, "DBT" "\0" "",
      { 0x50c8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5 } },
    { (struct token *)&tokimg.opdivu, fnum+53, "DIVS" "\0" "",
      { 0x81c0, 0xffd, dn_, (0<<4)+3, (9<<4)+15, (UWORD)((15<<12)+word_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opeor, fnum+54, "DIVU" "\0" "",
      { 0x80c0, 0xffd, dn_, (0<<4)+3, (9<<4)+15, (UWORD)((15<<12)+word_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opeori, fnum+55, "EOR" "\0" "",
      { 0xb100, dn_, 0x1fd, (9<<4)+15, (0<<4)+3, (UWORD)((6<<12)+sall_), NULL, (WORD)0x8002 } },
    { (struct token *)&tokimg.opexg, fnum+56, "EORI" "\0" "",
      { 0xa00, imm_, 0x1fd^ccr_^sr_^imm_, (15<<4)+15, (0<<4)+3, (UWORD)((6<<12)+sall_), andichk, (WORD)0 } },
    { (struct token *)&tokimg.opext, fnum+57, "EXG" "\0" "",
      { 0xc100, dn_^an_, dn_^an_, (9<<4)+15, (0<<4)+15, (UWORD)((15<<12)+long_), exgchk, (WORD)0 } },
    { (struct token *)&tokimg.opillegal, fnum+58, "EXT" "\0" "",
      { 0x4800, dn_, 0, (0<<4)+15, (15<<4)+15, (UWORD)((6<<12)+0x32f), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opjmp, fnum+59, "ILLEGAL" "\0" "",
      { 0x4afc, 0, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opjsr, fnum+60, "JMP" "\0" "",
      { 0x4ec0, 0x7e4, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.oplea, fnum+61, "JSR" "\0" "",
      { 0x4e80, 0x7e4, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.oplink, fnum+62, "LEA" "\0" "",
      { 0x41c0, 0x7e4, an_, (0<<4)+3, (9<<4)+15, (UWORD)((15<<12)+long_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.oplsl, fnum+63, "LINK" "\0" "",
      { 0x4e50, an_, imm_, (0<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.oplsr, fnum+64, "LSL" "\0" "",
      { 0xe108, imm_^0x1fd, 1, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+sall_), lslchk, (WORD)1 } },
    { (struct token *)&tokimg.opmove, fnum+65, "LSR" "\0" "",
      { 0xe008, imm_^0x1fd, 1, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+sall_), lslchk, (WORD)1 } },
    { (struct token *)&tokimg.opmovep, fnum+66, "MOVE" "\0" "",
      { 0, all_^ccr_^sr_^usp_, 0x1fd^ccr_^sr_^usp_, (0<<4)+3, (9<<4)+6, (UWORD)((15<<12)+sall_), movechk, (WORD)0x8001 } },
    { (struct token *)&tokimg.opmovea, fnum+67, "MOVEP" "\0" "",
      { 0x108, dn_^dan_, dn_^dan_, (9<<4)+15, (0<<4)+15, (UWORD)((6<<12)+0x10f), movepchk, (WORD)0 } },
    { (struct token *)&tokimg.opmoveq, fnum+68, "MOVEA" "\0" "",
      { 0x40, all_, an_, (0<<4)+3, (9<<4)+15, (UWORD)((12<<12)+0x23f), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opmovem, fnum+69, "MOVEQ" "\0" "",
      { 0x7000, imm_, dn_, (15<<4)+15, (9<<4)+15, (UWORD)((15<<12)+long_), NULL, (WORD)3 } },
    { (struct token *)&tokimg.opmuls, fnum+70, "MOVEM" "\0" "",
      { 0x4880, mem_^0x7fc, mem_^0x7fc, (0<<4)+3, (0<<4)+3, (UWORD)((6<<12)+0x10f), movemchk, (WORD)0 } },
    { (struct token *)&tokimg.opmulu, fnum+71, "MULS" "\0" "",
      { 0xc1c0, 0xffd, dn_, (0<<4)+3, (9<<4)+15, (UWORD)((15<<12)+word_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opnbcd, fnum+72, "MULU" "\0" "",
      { 0xc0c0, 0xffd, dn_, (0<<4)+3, (9<<4)+15, (UWORD)((15<<12)+word_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opneg, fnum+73, "NBCD" "\0" "",
      { 0x4800, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opnegx, fnum+74, "NEG" "\0" "",
      { 0x4400, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((6<<12)+sall_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opnop, fnum+75, "NEGX" "\0" "",
      { 0x4000, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((6<<12)+sall_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opnot, fnum+76, "NOP" "\0" "",
      { 0x4e71, 0, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opor, fnum+77, "NOT" "\0" "",
      { 0x4600, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((6<<12)+sall_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opori, fnum+78, "OR" "\0" "",
      { 0x8000, all_&nan_, 0x1fd, (0<<4)+3, (9<<4)+15, (UWORD)((6<<12)+sall_), addchk, (WORD)0x8002 } },
    { (struct token *)&tokimg.oppea, fnum+79, "ORI" "\0" "",
      { 0, imm_, 0x1fd^ccr_^sr_^imm_, (15<<4)+15, (0<<4)+3, (UWORD)((6<<12)+sall_), andichk, (WORD)0 } },
    { (struct token *)&tokimg.opreset, fnum+80, "PEA" "\0" "",
      { 0x4840, 0x7e4, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+long_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.oprol, fnum+81, "RESET" "\0" "",
      { 0x4e70, 0, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opror, fnum+82, "ROL" "\0" "",
      { 0xe118, imm_^0x1fd, 1, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+sall_), rolchk, (WORD)1 } },
    { (struct token *)&tokimg.oproxl, fnum+83, "ROR" "\0" "",
      { 0xe018, imm_^0x1fd, 1, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+sall_), rolchk, (WORD)1 } },
    { (struct token *)&tokimg.oproxr, fnum+84, "ROXL" "\0" "",
      { 0xe110, imm_^0x1fd, 1, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+sall_), roxlchk, (WORD)1 } },
    { (struct token *)&tokimg.oprte, fnum+85, "ROXR" "\0" "",
      { 0xe010, imm_^0x1fd, 1, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+sall_), roxlchk, (WORD)1 } },
    { (struct token *)&tokimg.oprtr, fnum+86, "RTE" "\0" "",
      { 0x4e73, 0, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.oprts, fnum+87, "RTR" "\0" "",
      { 0x4e77, 0, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opsbcd, fnum+88, "RTS" "\0" "",
      { 0x4e75, 0, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opscc, fnum+89, "SBCD" "\0" "",
      { 0x8100, dn_^pan_, dn_^pan_, (9<<4)+0, (0<<4)+0, (UWORD)((15<<12)+byte_), abcdchk, (WORD)0 } },
    { (struct token *)&tokimg.opscs, fnum+90, "SCC" "\0" "",
      { 0x54c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opseq, fnum+91, "SCS" "\0" "",
      { 0x55c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opsge, fnum+92, "SEQ" "\0" "",
      { 0x57c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opsgt, fnum+93, "SGE" "\0" "",
      { 0x5Cc0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opshi, fnum+94, "SGT" "\0" "",
      { 0x5Ec0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opsle, fnum+95, "SHI" "\0" "",
      { 0x52c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opsls, fnum+96, "SLE" "\0" "",
      { 0x5Fc0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opslt, fnum+97, "SLS" "\0" "",
      { 0x53c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opsmi, fnum+98, "SLT" "\0" "",
      { 0x5Dc0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opsne, fnum+99, "SMI" "\0" "",
      { 0x5Bc0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opspl, fnum+100, "SNE" "\0" "",
      { 0x56c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opsvc, fnum+101, "SPL" "\0" "",
      { 0x5Ac0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opsvs, fnum+102, "SVC" "\0" "",
      { 0x58c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opst, fnum+103, "SVS" "\0" "",
      { 0x59c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opsf, fnum+104, "ST" "\0" "",
      { 0x50c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opstop, fnum+105, "SF" "\0" "",
      { 0x51c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opsub, fnum+106, "STOP" "\0" "",
      { 0x4e72, imm_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opsubi, fnum+107, "SUB" "\0" "",
      { 0x9000, all_, 0x1fd, (0<<4)+3, (9<<4)+15, (UWORD)((6<<12)+sall_), addchk, (WORD)0x8003 } },
    { (struct token *)&tokimg.opsuba, fnum+108, "SUBI" "\0" "",
      { 0x400, imm_, 0x1fd, (15<<4)+15, (0<<4)+3, (UWORD)((6<<12)+sall_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opsubq, fnum+109, "SUBA" "\0" "",
      { 0x90c0, all_, 2, (0<<4)+3, (9<<4)+15, (UWORD)((8<<12)+wordlong_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opsubx, fnum+110, "SUBQ" "\0" "",
      { 0x5100, imm_, 0x1ff, (15<<4)+15, (0<<4)+3, (UWORD)((6<<12)+sall_), NULL, (WORD)1 } },
    { (struct token *)&tokimg.opswap, fnum+111, "SUBX" "\0" "",
      { 0x9100, dn_^pan_, dn_^pan_, (0<<4)+15, (9<<4)+15, (UWORD)((6<<12)+sall_), abcdchk, (WORD)0 } },
    { (struct token *)&tokimg.optas, fnum+112, "SWAP" "\0" "",
      { 0x4840, dn_, 0, (0<<4)+15, (15<<4)+15, (UWORD)((15<<12)+word_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.optrap, fnum+113, "TAS" "\0" "",
      { 0x4ac0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.optrapv, fnum+114, "TRAP" "\0" "",
      { 0x4e40, imm_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)4 } },
    { (struct token *)&tokimg.optst, fnum+115, "TRAPV" "\0" "",
      { 0x4e76, 0, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.opunlk, fnum+116, "TST" "\0" "",
      { 0x4a00, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((6<<12)+sall_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.endasm, fnum+117, "UNLK" "\0" "",
      { 0x4e58, an_, 0, (0<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0 } },
    { (struct token *)&tokimg.toktnum, fnum+118, "EndAsm" "\0" "" },
    { (struct token *)&tokimg.toktnum2, tnum, "List" "\0" "" },
    { (struct token *)&tokimg.toktnum3, tnum+1, "SetErr" "\0" "" },
    { (struct token *)&tokimg.toktnum5, tnum+2, "ClrErr" "\0" "" },
    { (struct token *)&tokimg.toktnum6, tnum+4, "ErrFail" "\0" "" },
    { (struct token *)&tokimg.toktnum7, tnum+5, "Addr" "\0" "Objectname(Object#)" },
    { (struct token *)&tokimg.toktnum8, tnum+6, "MOD" "\0" "" },
    { (struct token *)&tokimg.toktnum9, tnum+7, "Pi" "\0" "" },
    { (struct token *)&tokimg.toktnum10, tnum+8, "Repeat" "\0" "" },
    { (struct token *)&tokimg.toktnum11, tnum+9, "Until" "\0" "Expression" },
    { (struct token *)&tokimg.toktnum12, tnum+10, "PutReg" "\0" "68000 Reg,Variable" },
    { (struct token *)&tokimg.toktnum13, tnum+11, "Pop" "\0" "Gosub|For|Select|If|While|Repeat" },
    { (struct token *)&tokimg.toktnum14, tnum+12, "INCDIR" "\0" "Pathname" },
    { (struct token *)&tokimg.toktnum15, tnum+13, "EndIf" "\0" "" },
    { (struct token *)&tokimg.toktnum16, tnum+14, "Wend" "\0" "" },
    { (struct token *)&tokimg.toktnum17, tnum+15, "SysJsr" "\0" "Routine" },
    { (struct token *)&tokimg.toktnum18, tnum+16, "WBStartup" "\0" "" },
    { (struct token *)&tokimg.toktnum19, tnum+17, "Maximum" "\0" "Objectname" },
    { (struct token *)&tokimg.toktnum20, tnum+18, "On" "\0" "Expression Goto|Gosub Program Label[,Program Label...]" },
    { (struct token *)&tokimg.toktnum21, tnum+19, "Off" "\0" "" },
    { (struct token *)&tokimg.toktnum22, tnum+20, "Forever" "\0" "" },
    { (struct token *)&tokimg.toktnum23, tnum+21, "Restore" "\0" "[Program Label]" },
    { (struct token *)&tokimg.toktnum24, tnum+22, "Exchange" "\0" "Var,Var" },
    { (struct token *)&tokimg.toktnum25, tnum+23, "USEPATH" "\0" "Pathtext" },
    { (struct token *)&tokimg.toktnum26, tnum+24, "CloseEd" "\0" "" },
    { (struct token *)&tokimg.toktnum27, tnum+25, "NoCli" "\0" "" },
    { (struct token *)&tokimg.toktnum28, tnum+26, "BitTst" "\0" "" },
    { (struct token *)&tokimg.toktnum29, tnum+27, "BitSet" "\0" "" },
    { (struct token *)&tokimg.toktnum30, tnum+28, "BitClr" "\0" "" },
    { (struct token *)&tokimg.toktnum31, tnum+29, "BitChg" "\0" "" },
    { (struct token *)&tokimg.toktnum32, tnum+30, "Used" "\0" "Objectname(Object#)" },
    { (struct token *)&tokimg.toktnum33, tnum+31, "Runerrson" "\0" "" },
    { NULL, tnum+32, "Runerrsoff" "\0" "" },
};

/* asm label `tokens` = first node: macro in main.h */

/* 274 edstruct: copied (28 bytes) by the editor into its own block */
struct edstruct edstruct = {
    (struct token *)&tokimg,      /* tokens */
    menus,
    (APTR)loadxtra_stub,        /* load .xtra routine */
    (APTR)savextra_stub,        /* save .xtra routine */
    (APTR)menu_stub,            /* Menu Routine */
    NULL,                       /* cleanup (?) */
    { ':', '"', '.', ';' }
};

/* 724 menus: "COMPILER\0", items "TEXT\0" + shortcut char, 0 */
const char menus[] =
    "COMPILER" "\0"
    "COMPILE AND RUN        " "\0" "X"
    "RUN                    " "\0" "M"
    "CREATE EXECUTABLE      " "\0" "E"
    "COMPILER OPTIONS       " "\0" "O"
    "CREATE RESIDENT        " "\0" ";"
    "VIEW NEWTYPES          " "\0" "-"
    "CLI ARGUMENT           " "\0" "="
    "CALCULATOR             " "\0" "H"
    "RELOAD ALL LIBS        " "\0" "\\"
    "LOAD DEBUG MODULE      " "\0" "."
#if !final
    "PRINT TOKENS" "\0" "\0"
#endif
    ;   /* + the implicit terminating 0 = asm `dc.b 0` */

/* 741 menuadds: menu dispatch (index = editor item number) */
static void (*const menuadds[])(void) = {
    menu0, menu10, menu1, menu4, menu5,
    menu6, menu9, menu7, menu8, menu_d,
#if !final
    menu3,
#endif
};

/* 748 */
UBYTE undobuffer[128] __attribute__((aligned(4)));

/* 750 */
char libsdir[] = "blitz2:userlibs/";

/* ================================================== hunk templates (265) */

ULONG tsthead[12] = {
    0x3f3, 0, 1, 0, 0,          /* tsthead: HUNK_HEADER, no names, 1 hunk */
    256, 0x3e9,                 /* tstsize1, HUNK_CODE */
    256,                        /* tstsize2 */
    0x3ec,                      /* tstend: HUNK_RELOC32 */
    0, 0,                       /* tstsize3, hunk 0 */
    0x3f2                       /* tstdone: HUNK_END */
};

/* ================================================== requesters (753-912)
   Templates for the editor's openreq (window size from Width/Height). */

static WORD nomemreqla1[] = { 303,0,0,0,0,67 };
static WORD nomemreqla3[] = { 303,1,303,67,1,67 };
static WORD nomemreqla5[] = { 207,0,0,0,0,11 };
static WORD nomemreqla7[] = { 207,1,207,11,1,11 };
static WORD nomemreqla9[] = { 207,0,0,0,0,11 };
static WORD nomemreqla11[] = { 207,1,207,11,1,11 };
static WORD nomemreqla13[] = { 119,0,0,0,0,11 };
static WORD nomemreqla15[] = { 119,1,119,11,1,11 };
static struct Border nomemreqla2 = { 0,0, 2,1,1,3, nomemreqla3, NULL };
static struct Border nomemreqbo1 = { 0,0, 1,2,1,3, nomemreqla1, &nomemreqla2 };
static struct Border nomemreqla6 = { 0,0, 2,1,1,3, nomemreqla7, NULL };
static struct Border nomemreqbo2 = { 0,0, 1,2,1,3, nomemreqla5, &nomemreqla6 };
static struct Border nomemreqla10 = { 0,0, 2,1,1,3, nomemreqla11, NULL };
static struct Border nomemreqbo3 = { 0,0, 1,2,1,3, nomemreqla9, &nomemreqla10 };
static struct Border nomemreqla14 = { 0,0, 2,1,1,3, nomemreqla15, NULL };
static struct Border nomemreqbo4 = { 0,0, 1,2,1,3, nomemreqla13, &nomemreqla14 };
static struct IntuiText nomemreqin1 = { 1,0,1, 12,6, NULL,
    (UBYTE *)"Not Enough Object Memory Available!", NULL };
static struct IntuiText nomemreqin2 = { 1,0,1, 4,2, NULL,
    (UBYTE *)"INCREASE 150% - RECOMPILE", NULL };
static struct IntuiText nomemreqin3 = { 1,0,1, 4,2, NULL,
    (UBYTE *)"MAKE SMALLEST - RECOMPILE", NULL };
static struct IntuiText nomemreqin4 = { 1,0,1, 4,2, NULL,
    (UBYTE *)"CANCEL COMPILE", NULL };
static struct Gadget nomemreqga3 = { NULL, 96,52,120,12, 0,5,1,
    &nomemreqbo4, NULL, &nomemreqin4, 0, NULL, 3, NULL };
static struct Gadget nomemreqga2 = { &nomemreqga3, 48,36,208,12, 0,5,1,
    &nomemreqbo3, NULL, &nomemreqin3, 0, NULL, 2, NULL };
static struct Gadget nomemreqga1 = { &nomemreqga2, 48,20,208,12, 0,5,1,
    &nomemreqbo2, NULL, &nomemreqin2, 0, NULL, 1, NULL };
struct Requester nomemreq = { NULL, 116,58,304,68, 0,0,
    &nomemreqga1, &nomemreqbo1, &nomemreqin1, 0, 0 };

/* 837 asmpnts: defined in stmts.c (chunk 03) */

static WORD verreqla1[] = { 279,0,0,0,0,59 };
static WORD verreqla3[] = { 279,1,279,59,1,59 };
static WORD verreqla7[] = { 103,0,0,0,0,11 };
static WORD verreqla9[] = { 103,1,103,11,1,11 };
static struct Border verreqla2 = { 0,0, 2,1,1,3, verreqla3, NULL };
static struct Border verreqbo1 = { 0,0, 1,2,1,3, verreqla1, &verreqla2 };
static struct Border verreqla8 = { 0,0, 2,1,1,3, verreqla9, NULL };
static struct Border verreqbo2 = { 0,0, 1,2,1,3, verreqla7, &verreqla8 };
/* 870 verreqla4: digits computed from `version` at assembly time */
static char verreqla4[] = { 'B','l','i','t','z',' ','B','a','s','i','c',' ',
    '2',' ','V','e','r','s','i','o','n',' ',
    '0' + blitz2version / 100, '.', '0' + blitz2version % 100 / 10,
    '0' + blitz2version % 10, 0 };
#if demo
static struct IntuiText verreqin3 = { 1,0,1, 64-8,30, NULL,
    (UBYTE *)"Demonstartion Version", NULL };
#endif
#if release
static struct IntuiText verreqin3 = { 1,0,1, 64+16,30, NULL,
    (UBYTE *)"Release Version", NULL };
#endif
static struct IntuiText verreqin2 = { 1,0,1, 12,18, NULL,
    (UBYTE *)"(C)opyright 1995 - Acid Software", &verreqin3 };
static struct IntuiText verreqin1 = { 1,0,1, 36,6, NULL,
    (UBYTE *)verreqla4, &verreqin2 };
static struct IntuiText verreqin4 = { 1,0,1, 4,2, NULL,
    (UBYTE *)" OKEE DOKEE ", NULL };
static struct Gadget verreqga1 = { NULL, 92,44,104,12, 0,5,1,
    &verreqbo2, NULL, &verreqin4, 0, NULL, 1, NULL };
struct Requester verreq = { NULL, 108,58,280,60, 0,0,
    &verreqga1, &verreqbo1, &verreqin1, 0, 0 };

/* ================================================== objects (914-1254)
 * Some object codes for compile: copied into the generated program or
 * patched in place (see survey-01 D).  Bytes from the vasm listing,
 * verified against the shipped binary. */
/* 548 bytes */
UWORD objects[] = {
    0xE148,                            /* +0   lslimm lsl #8,d0 */
    0x266D, 0x0000,                    /* +2   use move.l 0(a5),a3 */
    0xD6C0,                            /* +6   add d0,a3 */
    0x2B4B, 0x0000,                    /* +8   move.l a3,0(a5) */
    0x203C, 0x0000, 0x0000,            /* +12  beginarr move.l #0,d0 */
    0xC0C1,                            /* +18  mularr mulu d1,d0 */
    0x2B40, 0x0000,                    /* +20  putarr move.l d0,0(a5) */
    0x343C, 0x0000,                    /* +24  doarr move #0,d2 */
    0x363C, 0x0000,                    /* +28  move #0,d3 */
    0x4A40,                            /* +32  tstword tst.w d0 */
    0x4A80,                            /* +34  tstlong tst.l d0 */
    0x4E71,                            /* +36  nops8 nop */
    0x4E71,                            /* +38  nop */
    0x4E71,                            /* +40  nop */
    0x4E71,                            /* +42  nop */
    0x4E71,                            /* +44  nops4 nop */
    0x4E71,                            /* +46  nops3 nop */
    0x4E71,                            /* +48  nop */
    0x4E71,                            /* +50  nop */
    0x266D, 0x0000,                    /* +52  dataget move.l 0(a5),a3 */
    0x2B4B, 0x0000,                    /* +56  dataput move.l a3,0(a5) */
    0x2F1B,                            /* +60  stlenget move.l (a3)+,-(a7) */
    0x524B,                            /* +62  stalign addq #1,a3 */
    0xC18B,                            /* +64  exg d0,a3 */
    0x0880, 0x0000,                    /* +66  bclr #0,d0 */
    0xC18B,                            /* +70  exg d0,a3 */
    0x524B,                            /* +72  dataletb addq #1,a3 */
    0x1B5B, 0x0000,                    /* +74  move.b (a3)+,0(a5) */
    0x524B,                            /* +78  dataletbl addq #1,a3 */
    0x195B, 0x0000,                    /* +80  move.b (a3)+,0(a4) */
    0x524B,                            /* +84  dataletb2 addq #1,a3 */
    0x149B,                            /* +86  move.b (a3)+,(a2) */
    0x3B5B, 0x0000,                    /* +88  dataletw move (a3)+,0(a5) */
    0x349B,                            /* +92  move (a3)+,(a2) */
    0x395B, 0x0000,                    /* +94  move (a3)+,0(a4) */
    0x2B5B, 0x0000,                    /* +98  dataletl move.l (a3)+,0(a5) */
    0x249B,                            /* +102 move.l (a3)+,(a2) */
    0x295B, 0x0000,                    /* +104 move.l (a3)+,0(a4) */
    0x504F,                            /* +108 nextfixw addq #8,a7 */
    0x4FEF, 0x000A,                    /* +110 nextfixw2 lea 10(a7),a7 */
    0x4FEF, 0x000C,                    /* +114 nextfixl lea 12(a7),a7 */
    0x4FEF, 0x000E,                    /* +118 nextfixl2 lea 14(a7),a7 */
    0x246F, 0x0004,                    /* +122 move.l 4(a7),a2 */
    0x3017,                            /* +126 move (a7),d0 */
    0xD112,                            /* +128 add.b d0,(a2) */
    0x246F, 0x0004,                    /* +130 move.l 4(a7),a2 */
    0x3017,                            /* +134 move (a7),d0 */
    0xD152,                            /* +136 add d0,(a2) */
    0x246F, 0x0008,                    /* +138 move.l 8(a7),a2 */
    0x2017,                            /* +142 move.l (a7),d0 */
    0xD192,                            /* +144 add.l d0,(a2) */
    0x246F, 0x0008,                    /* +146 move.l 8(a7),a2 */
    0x2012,                            /* +150 move.l (a2),d0 */
    0x2217,                            /* +152 move.l (a7),d1 */
    0x4EAE, 0xFFBE,                    /* +154 jsr -66(a6) */
    0x2480,                            /* +158 move.l d0,(a2) */
    0x246F, 0x0004,                    /* +160 move.l 4(a7),a2 */
    0x1012,                            /* +164 move.b (a2),d0 */
    0x322F, 0x0002,                    /* +166 move 2(a7),d1 */
    0x4A57,                            /* +170 tst (a7) */
    0x6A00, 0x0004,                    /* +172 bpl .skip */
    0xC141,                            /* +176 exg d0,d1 */
    0xB001,                            /* +178 .skip cmp.b d1,d0 */
    0x246F, 0x0004,                    /* +180 move.l 4(a7),a2 */
    0x3012,                            /* +184 move (a2),d0 */
    0x322F, 0x0002,                    /* +186 move 2(a7),d1 */
    0x4A57,                            /* +190 tst (a7) */
    0x6A00, 0x0004,                    /* +192 bpl .skip */
    0xC141,                            /* +196 exg d0,d1 */
    0xB041,                            /* +198 .skip cmp d1,d0 */
    0x246F, 0x0008,                    /* +200 move.l 8(a7),a2 */
    0x2012,                            /* +204 move.l (a2),d0 */
    0x222F, 0x0004,                    /* +206 move.l 4(a7),d1 */
    0x4A57,                            /* +210 tst (a7) */
    0x6A00, 0x0004,                    /* +212 bpl .skip */
    0xC141,                            /* +216 exg d0,d1 */
    0xB081,                            /* +218 .skip cmp.l d1,d0 */
    0x246F, 0x0008,                    /* +220 move.l 8(a7),a2 */
    0x2012,                            /* +224 move.l (a2),d0 */
    0x222F, 0x0004,                    /* +226 move.l 4(a7),d1 */
    0x082F, 0x0007, 0x0003,            /* +230 btst #7,3(a7) */
    0x6700, 0x0004,                    /* +236 beq .skip */
    0xC141,                            /* +240 exg d0,d1 */
    0x4EAE, 0xFFD6,                    /* +242 jsr -42(a6) */
    0x4840,                            /* +246 swapd0 swap d0 */
    0x2F0A,                            /* +248 pushindex move.l a2,-(a7) */
    0x45ED, 0x0000,                    /* +250 leaamp lea 0(a5),a2 */
    0x200A,                            /* +254 leaamp2 move.l a2,d0 */
    0x45EC, 0x0000,                    /* +256 leaampl lea 0(a4),a2 */
    0x2012,                            /* +260 stamp move.l (a2),d0 */
    0x246D, 0x0000,                    /* +262 stamp2 move.l 0(a5),a2 */
    0x584F,                            /* +266 stol addq #4,a7 */
    0x3F00,                            /* +268 pushd0wd move d0,-(a7) */
    0x2F00,                            /* +270 pushd0l move.l d0,-(a7) */
    0x3E3C, 0x0000,                    /* +272 movenumrep move #0,d7 */
    0x45EF, 0x0000,                    /* +276 preplibst lea 0(a7),a2 */
    0x4FEF, 0x0000,                    /* +280 fixlibst lea 0(a7),a7 */
    0x2F00,                            /* +284 pushdn move.l d0,-(a7) */
    0x2F0B,                            /* +286 movea3a7 move.l a3,-(a7) */
    0x202F, 0x0006,                    /* +288 movestst2 move.l 6(a7),d0 */
    0x3F6F, 0x0004, 0x0008,            /* +292 move 4(a7),8(a7) */
    0x2E9F,                            /* +298 move.l (a7)+,(a7) */
    0x202F, 0x0004,                    /* +300 movea7dn move.l 4(a7),d0 */
    0x2E9F,                            /* +304 move.l (a7)+,(a7) */
    0x2640,                            /* +306 moved0a3 move.l d0,a3 */
    0x202D, 0x0000,                    /* +308 movestdn move.l 0(a5),d0 */
    0x200B,                            /* +312 movea3dn move.l a3,d0 */
    0x2F00,                            /* +314 putstlen move.l d0,-(a7) */
    0x201F,                            /* +316 getstlen move.l (a7)+,d0 */
    0x2F57, 0x0000,                    /* +318 moverts move.l (a7),0(a7) */
    0x4FEF, 0x0000,                    /* +322 fixstack lea 0(a7),a7 */
    0x504F,                            /* +326 fixq addq #8,a7 */
    0x2E9F,                            /* +328 fix1 move.l (a7)+,(a7) */
    0x2640,                            /* +330 movedna3 move.l d0,a3 */
    0x45EC, 0x0000,                    /* +332 least lea 0(a4),a2 */
    0x2F2C, 0x0000,                    /* +336 funcst move.l 0(a4),-(a7) */
    0x584A,                            /* +340 addq4a2 addq #4,a2 */
    0x2F0B,                            /* +342 savesbase move.l a3,-(a7) */
    0x2B4B, 0x0000,                    /* +344 savesbase2 move.l a3,0(a5) */
    0x265F,                            /* +348 ressbase move.l (a7)+,a3 */
    0x2B4B, 0x0000,                    /* +350 ressbase2 move.l a3,0(a5) */
    0xCB4C,                            /* +354 exga4a5 exg.l a4,a5 (Devpac encoding; vasm: C94D) */
    0x4E54, 0x0000,                    /* +356 linka4 link a4,#0 */
    0x4E5C,                            /* +360 unlinka4 unlk a4 */
    0x2F0A,                            /* +362 a2toa7 move.l a2,-(a7) */
    0x245F,                            /* +364 a7toa2 move.l (a7)+,a2 */
    0x4EF9, 0x0000, 0x0000,            /* +366 gotocode jmp 0 */
    0x4EB9, 0x0000, 0x0000,            /* +372 gosubcode jsr 0 */
    0x0839, 0x0006, 0x00BF, 0xE001,    /* +378 mwait btst #6,$bfe001 */
    0x6600, 0xFFF6,                    /* +386 bne mwait */
    0x2000,                            /* +390 libreg move.l d0,d0 */
    0x2C6D, 0x0000,                    /* +392 libbase move.l 0(a5),a6 */
    0x4EAE, 0x0000,                    /* +396 libjsr jsr 0(a6) */
    0x202D, 0x0000,                    /* +400 regtransd move.l 0(a5),d0 */
    0x206D, 0x0000,                    /* +404 regtransa move.l 0(a5),a0 */
    0x267C, 0x0000, 0x0000,            /* +408 litget move.l #0,a3 */
    0x3F1B,                            /* +414 litget2 move (a3)+,-(a7) */
    0x2012,                            /* +416 movea2dn move.l (a2),d0 */
    0x202D, 0x0000,                    /* +418 moved3a5dn move.l 0(a5),d0 */
    0x7000,                            /* +422 stvarget2 moveq #0,d0 */
    0x2F00,                            /* +424 move.l d0,-(a7) */
    0x4A92,                            /* +426 tst.l (a2) */
    0x6700, 0x000E,                    /* +428 beq varget2skip */
    0x2052,                            /* +432 move.l (a2),a0 */
    0x5948,                            /* +434 subq #4,a0 */
    0x2E98,                            /* +436 move.l (a0)+,(a7) */
    0x4EB9, 0x0000, 0x0000,            /* +438 jsr 0 */
    0x201F,                            /* +444 addcode move.l (a7)+,d0 */
    0xD197,                            /* +446 add.l d0,(a7) */
    0x2F18,                            /* +448 pushlen move.l (a0)+,-(a7) */
    0x45ED, 0x0000,                    /* +450 leaa5d3a2 lea 0(a5),a2 */
    0x2F0A,                            /* +454 pusha2 move.l a2,-(a7) */
    0x2457,                            /* +456 geta2 move.l (a7),a2 */
    0x245F,                            /* +458 pulla2 move.l (a7)+,a2 */
    0x246F, 0x0004,                    /* +460 pulla2st move.l 4(a7),a2 */
    0x2E9F,                            /* +464 move.l (a7)+,(a7) */
    0x2F2D, 0x0000,                    /* +466 pusha3 move.l 0(a5),-(a7) */
    0x2B4B, 0x0000,                    /* +470 pusha32 move.l a3,0(a5) */
    0x2B5F, 0x0000,                    /* +474 pulla3 move.l (a7)+,0(a5) */
    0x266D, 0x0000,                    /* +478 geta3 move.l 0(a5),a3 */
    0x207C, 0x0000, 0x0000,            /* +482 numtoa0 move.l #0,a0 */
    0x247C, 0x0000, 0x0000,            /* +488 numtoa2 move.l #0,a2 */
    0x383C, 0x0000,                    /* +494 endarray move #0,d4 */
    0x343C, 0x0000,                    /* +498 preparray move #0,d2 */
    0x363C, 0x0000,                    /* +502 move #0,d3 */
    0x203C, 0x0000, 0x0004,            /* +506 dimbegin move.l #4,d0 */
    0x2B40, 0x0000,                    /* +512 dimb4 move.l d0,0(a5) */
    0xC0C1,                            /* +516 dimaf mulu d1,d0 */
    0x2B40, 0x0000,                    /* +518 dimfin move.l d0,0(a5) */
    0x2452,                            /* +522 movea0 move.l (a2),a2 */
    0xD5CA,                            /* +524 adda0a0 add.l a2,a2 */
    0xD5CA,                            /* +526 add.l a2,a2 */
    0x2C78, 0x0004,                    /* +528 getebase move.l 4.w,a6 */
    0x246D, 0x0000,                    /* +532 movea5a0 move.l 0(a5),a2 */
    0x246A, 0x0000,                    /* +536 movea0a0 move.l 0(a2),a2 */
    0x524A,                            /* +540 addqa0 addq #1,a2 */
    0x45EA, 0x0000,                    /* +542 leaa0a0 lea 0(a2),a2 */
    0xD4C0,                            /* +546 addrega2 add d0,a2 */
};

char tstname[] = "ram:test";            /* 1256 (unused) */

/* -----------end of object codes!-------------- */

/* ================================================== globals (1261-1296) */

static ULONG memat __attribute__((unused));                     /* 1261 (unused) */
static UWORD memlen __attribute__((unused));                    /* 1262 (unused); memstackf (1264):
                                           label without storage */
/* 1265 maths -> MathBase (above) */
UWORD regat;                            /* 1266 register being used! */
UWORD typelen;                          /* 1267 */
struct type *prevtype;                  /* 1268 */
UBYTE *prevloc;                         /* 1269 */
UWORD lastchar;                         /* 1270 */
struct type *thistype;                  /* 1271 */
/* 1272 errstack, 1273 errcont: replaced by errjmp/errmsg (parse.h) */
WORD linemode;                          /* 1274 0 if norm, 1 if newtype */
UWORD linesleft;                        /* 1275 */
struct editem *thisitem;                /* 1276 */
struct var *firstvar;                   /* 1277 */
struct alllocal *alllocals;             /* 1278 */
struct var *firstlocal;                 /* 1279 */
struct var *firstglob;                  /* 1280 */
WORD varoff;                            /* 1281 */
WORD locvaroff;                         /* 1282 */
struct editem *firstitem;               /* 1283 */
UWORD numlines;                         /* 1284 */
WORD returncode;                        /* 1285 */
struct ifnode *firstif;                 /* 1286 */

BPTR segadd;                            /* 1288 */
static char segname[] = "blitz2:ted";   /* 1289 */
static char mathsname[] = "mathffp.library";    /* 1291 */
static char debugname[] = "blitz2:defaultdbug"; /* 1483 */

/* 1294 cifbras -> tokens1.c (chunk 08), 1298 atokens -> compile.c (02) */

/* ================================================== begin (1353) */

/* blitz2.s:1353 actualbegin / 1357 begin - OKAY, Here it is - The actual
   compiler!  Returns the asm returncode (see main for what DOS gets). */
static WORD begin(void)
{
    struct m68kregs r;
    struct token *a0;
    APTR edmain;
    WORD d6;
    UWORD d0 = 0;

    /* find out how many system tokens exist */
    a0 = &tokens;
    for (;;) {
        d0++;                           /* count is not used */
        if (!a0->next)
            break;
        a0 = a0->next;
    }
    (void)d0;
    lasttoken = (ULONG)a0;
    makeasmtab();
    optreqprep();
    /* @1369 */
    returncode = -1;
    MathBase = OldOpenLibrary((CONST_STRPTR)mathsname);
    if (!MathBase)
        goto fail0;
    /* @1376 */
    constpcat = (ULONG)AllocMem(constmaxpc, MEMF_PUBLIC);
    if (!constpcat)
        goto failme;
    constlibat = constpcat + constmaxpc;
    /* @1384 */
    macrobuff = AllocMem(macrolen, MEMF_PUBLIC);
    if (!macrobuff)
        goto fail2;
    macrobufff = macrobuff + macrolen;
    /* @1392 load the editor and call its init (seg+8 = ted `outside`):
       a1=edstruct in; Z=1 ok, a0=editor main entry, a1=comdata out */
    segadd = LoadSeg((CONST_STRPTR)segname);
    if (!segadd)
        goto fail2a;
    memset(&r, 0, sizeof(r));
    r.a[1] = (ULONG)&edstruct;
    if (!(callregs((UBYTE *)BADDR(segadd) + 8, &r) & CCR_Z))
        goto fail3;
    edmain = (APTR)r.a[0];              /* asm: move.l a0,-(a7) */
    comdata = (APTR *)r.a[1];
    /* @1405 */
    returncode = 0;
    IntuitionBase = (struct IntuitionBase *)CD_L(CD_EXTINT);
    ed_openreq(&verreq);
    /* @1411 */
    sleepon();
    linenumat = -1;
    makelibs();
    loadlibs();
    makeiallox();
    pointeron();
    /* click cancel of version requester... */
    do {
        ;
    } while (!ed_getinput(&d6) || d6 != 1);
    /* grab a panic bit (failure of AllocSignal not checked, as in the
       original: not observable, #18 closed) */
    panicbit = (ULONG)(LONG)AllocSignal(-1);
    panicmask = 1UL << (panicbit & 31);
    /* loadseg default debugger (blitz2:defaultdbug) */
    dseg = LoadSeg((CONST_STRPTR)debugname);
    /* goto editor with filename! (returns when the user quits) */
    memset(&r, 0, sizeof(r));
    r.a[0] = (ULONG)progfile;
    callregs(edmain, &r);
    /* @1448 .skip */
    FreeSignal((LONG)panicbit);
    if (dseg)
        UnLoadSeg(dseg);
    freeemup();
    freeres();
    freelibs();
fail3:                                  /* 1460 */
    UnLoadSeg(segadd);
fail2a:                                 /* 1462 */
    freeallox();
    /* fail2b 1463 - BUG(orig) (#336): freed with the current macrolen, not the
       size of the allocation (options loaded later change macrolen) */
    FreeMem(macrobuff, macrolen);
fail2:                                  /* 1467 closedos */
    /* DIFF: asm closes dos.library here; libnix owns DOSBase and closes
       it at exit.  fail1a/fail1 (1468/1469): */
    FreeMem((APTR)constpcat, constmaxpc);
failme:                                 /* 1474 */
    CloseLibrary(MathBase);
fail0:                                  /* 1477 */
    optreqdone();
    flushlibs();
    return returncode;
}

/* ================================================== main (wbheader) */

/* wbheader:4 wb_header / fromcli / wb_exit + blitz2.s 253-261 mainbit:
   to be included to allow things to be run from WBENCH.  Gets the stack
   size (into asm d2 -> stackfuck) and the file to edit (progfile). */
int main(int argc, char **argv)
{
    ULONG d2;
    BPTR fh;

    us = FindTask(NULL);                /* asm: 276(execbase) = ThisTask */
    if (argc == 0) {
        /* program was run from workbench - somehow, we have to eventually
           get stacksize into D2!  DIFF: libnix already got the message
           (asm: WaitPort/GetMsg on pr_MsgPort) and replies it at exit. */
        struct WBArg *wa;

        wbmess = _WBenchMsg;
        wa = wbmess->sm_ArgList;
        /* Read in the .info file to find stack size! */
        CurrentDir(wa[0].wa_Lock);      /* dir of BLITZ2! */
        strcpy(progfile, (char *)wa[0].wa_Name);    /* make name+.info */
        strcat(progfile, wb_infoname);
        fh = Open((CONST_STRPTR)progfile, MODE_OLDFILE);
        d2 = 0;
        if (fh) {
            if (Read(fh, progfile, 0x4e) == 0x4e) {
                Close(fh);
                progfile[0] = 0;
                d2 = L_(progfile + 0x4a);           /* do_StackSize */
            }
            /* BUG(orig) (#19): on a short read the file is not closed */
        }
        /* BUG(orig) (#20): if the .info could not be opened progfile still holds
           "<name>.info" (after a short read: the partial .info bytes) and
           the editor is asked to load it */
        if (!d2)
            d2 = 4000;                  /* .openfailed */
        /* .openok: a project icon given? (asm: word at sm_NumArgs+2) */
        if ((UWORD)wbmess->sm_NumArgs >= 2) {
            CurrentDir(wa[1].wa_Lock);
            /* the old current dir lock is dropped as in the original
               (BUG(orig), #21).  DIFF: libnix's startup code restores
               the directory it saved before main at exit, so the port
               ends in the start directory, the original in the
               project's (or blitz2's) directory */
            strcpy(progfile, (char *)wa[1].wa_Name);
        }
    } else {
        /* fromcli: stacksize from CLI (asm: 4(a7) at entry) */
        struct Process *pr = (struct Process *)us;
        struct CommandLineInterface *cli = BADDR(pr->pr_CLI);

        d2 = cli ? (ULONG)cli->cli_DefaultStack * 4 : 4000;
        wbmess = NULL;
        /* DIFF: asm copied the raw argument line up to the first space or
           LF; libnix splits (and unquotes) it into argv */
        if (argc > 1) {
            strncpy(progfile, argv[1], sizeof(progfile) - 1);
            progfile[sizeof(progfile) - 1] = 0;
        }
    }
    /* DIFF: dosopen (wb:97) - libnix opens dos.library */
    /* wb_allgo / mainbit */
#if !final
    d2 = 4000;
#endif
    stackfuck = d2;
    begin();
    /* BUG(orig) (#22): wb_exit overwrites d0 (0 from the CLI; from the Workbench
       whatever ReplyMsg/Permit leave), so DOS never sees returncode */
    return 0;
}

/* ================================================== editor interface */

/* blitz2.s:1482 t, 1488 handler: empty labels */

static void ed_call(LONG off, struct m68kregs *r)
{
    callregs(CD_FN(off), r);
}

/* comdata-8 screenclose */
void ed_screenclose(void)
{
    struct m68kregs r = {{0}};
    ed_call(-8, &r);
}

/* comdata-4 screenopen2 */
void ed_screenopen2(void)
{
    struct m68kregs r = {{0}};
    ed_call(-4, &r);
}

/* comdata+0 openreq: a0 = requester template; d0 = 0 if not opened */
ULONG ed_openreq(struct Requester *req)
{
    struct m68kregs r = {{0}};
    r.a[0] = (ULONG)req;
    ed_call(0, &r);
    return r.d[0];
}

/* comdata+4 closereq */
void ed_closereq(void)
{
    struct m68kregs r = {{0}};
    ed_call(4, &r);
}

/* comdata+8 getinput: wait for input; d7=-1 gadget (d6 = GadgetID) */
BOOL ed_getinput(WORD *d6)
{
    struct m68kregs r = {{0}};
    ed_call(8, &r);
    *d6 = (WORD)r.d[6];
    return (WORD)r.d[7] == -1;
}

/* comdata+12 error: a0 = message, modal requester */
void ed_error(const char *msg)
{
    struct m68kregs r = {{0}};
    r.a[0] = (ULONG)msg;
    ed_call(12, &r);
}

/* comdata+16 detokallz */
void ed_detokallz(void)
{
    struct m68kregs r = {{0}};
    ed_call(16, &r);
}

/* comdata+20 tokall */
void ed_tokall(void)
{
    struct m68kregs r = {{0}};
    ed_call(20, &r);
}

/* comdata+24 ezfreq: a0 path, a1 name, a2 title; Z=1 cancel, else d0 */
char *ed_ezfreq(char *path, char *name, const char *title)
{
    struct m68kregs r = {{0}};
    r.a[0] = (ULONG)path;
    r.a[1] = (ULONG)name;
    r.a[2] = (ULONG)title;
    if (callregs(CD_FN(24), &r) & CCR_Z)
        return NULL;
    return (char *)r.d[0];
}

/* comdata+28 refgads: a0 gadget, d0 number */
void ed_refgads(struct Gadget *g, WORD n)
{
    struct m68kregs r = {{0}};
    r.a[0] = (ULONG)g;
    r.d[0] = (ULONG)(LONG)n;
    ed_call(28, &r);
}

/* comdata+32 getinput2: poll; returns d7 (0 = nothing), d6 = GadgetID */
LONG ed_getinput2(WORD *d6)
{
    struct m68kregs r = {{0}};
    ed_call(32, &r);
    *d6 = (WORD)r.d[6];
    return (LONG)r.d[7];
}

/* comdata+56 tokone: tokenise one line in a0 */
void ed_tokone(UBYTE *line)
{
    struct m68kregs r = {{0}};
    r.a[0] = (ULONG)line;
    ed_call(56, &r);
}

/* comdata+60 openio: open hendrix (program IO window) */
void ed_openio(void)
{
    struct m68kregs r = {{0}};
    ed_call(60, &r);
}

/* comdata+64 closeio */
void ed_closeio(void)
{
    struct m68kregs r = {{0}};
    ed_call(64, &r);
}

/* comdata+68 actgad: activate gadget in a0; d0 = ActivateGadget result */
ULONG ed_actgad(struct Gadget *g)
{
    struct m68kregs r = {{0}};
    r.a[0] = (ULONG)g;
    ed_call(68, &r);
    return r.d[0];
}

/* blitz2.s:1490 pointeron - jmp comdata+72 */
void pointeron(void)
{
    struct m68kregs r = {{0}};
    ed_call(72, &r);
}

/* blitz2.s:1494 sleepon - jmp comdata+76 */
void sleepon(void)
{
    struct m68kregs r = {{0}};
    ed_call(76, &r);
}

/* ================================================== menu handler */

/* blitz2.s:1498 menu - called by the editor (menu_stub): d6 = item,
   a5 = first line, d7 = number of lines; returns d0 = menuret */
void menu_c(struct m68kregs *r)
{
    firstitem = (struct editem *)r->a[5];
    numlines = (UWORD)r->d[7];
    linenumat = -1;
    menuret = -1;
    menuadds[(UWORD)r->d[6]]();
    /* asm: move menuret,d0 (word, upper half of d0 untouched) */
    r->d[0] = (r->d[0] & 0xffff0000UL) | (UWORD)menuret;
}

static char dtext[] = "Debug module to load";   /* 1509 */
static char dpath[7 + 192] = "Blitz2:";         /* 1511 */
static char dname[192];                         /* 1514 */
static const char *dname2;                      /* 1516 */
BPTR dseg;                                      /* 1485 */

/* blitz2.s:1518 menu_d - select debug module */
void menu_d(void)
{
    char *name = ed_ezfreq(dpath, dname, dtext);

    if (!name) {
        nodload();
        return;
    }
    loaddmod(name);                     /* falls through into loaddmod */
}

/* blitz2.s:1528 loaddmod - d0=name of module to load! */
void loaddmod(const char *name)
{
    jmp_buf jb, *olderr = errjmp;
    BPTR d7;

    dname2 = name;
    sleepon();
    errjmp = &jb;                       /* errstack=a7, errcont=nodload */
    if (setjmp(jb)) {
        errjmp = olderr;
        nodload();
        return;
    }
    d7 = LoadSeg((CONST_STRPTR)dname2);
    if (!d7)
        dloaderr();
    if (dseg) {
        UnLoadSeg(dseg);
        pointeron();
    }
    /* BUG(orig) (#23): without an old dseg the busy pointer stays on */
    dseg = d7;
    errjmp = olderr;                    /* DIFF: asm never restored errstack */
    nodload();
}

/* blitz2.s:1544 nodload */
void nodload(void)
{
}

/* blitz2.s:1546 dloaderr */
void dloaderr(void)
{
    err("Can't LoadSeg debug module!");
}

static char xtext[] = "Name of Executable to create";  /* 1550 */
char xpathname[384];                    /* 1552 xpath, 1553 xname (see main.h) */
char *execname;                         /* 1554 */

/* blitz2.s:1556 menu1 - compile/save */
void menu1(void)
{
    char *name = ed_ezfreq(xpath, xname, xtext);
    UWORD saved;

    if (!name)
        return;
    /* O.K.... Now to create it */
    execname = name;
    makeexec = -1;
    saved = optreqga15.Flags;
    optreqga15.Flags |= GFLG_SELECTED;  /* make small! (bset #7,+13) */
    compile();
    if (!anyerrs)
        savefile();
    optreqga15.Flags = saved;
}

WORD closeed;                           /* 1584 */
WORD nocli;                             /* 1585 */

/* blitz2.s:1578 docloseed - CloseEd statement */
void docloseed(UWORD c)
{
    (void)c;
    closeed = -1;
}

/* blitz2.s:1581 donocli - NoCli statement */
void donocli(UWORD c)
{
    (void)c;
    nocli = -1;
}

/* blitz2.s:1587 flushc - exec CacheClearU if the library has it */
void flushc(void)
{
    if (SysBase->LibNode.lib_NegSize >= 636)
        CacheClearU();
}

volatile BPTR tasklock;                 /* 1675 */
struct Task * volatile panictask;       /* 1676 */
ULONG panicbit;                         /* 1677 */
ULONG panicmask;                        /* 1678 */
static char procname[] = "Blitz ][ Program Proc";   /* 1680 */

/* DIFF: the new process enters runtheprog from CreateProc; with -fbaserel
   the C code needs a4 = the base of the globals, which the process does
   not have: __saveds loads it (#338) */
static __saveds void runtheprog(void);

/* 1683-1687 procstart: a fake segment list for CreateProc: length, next
   BPTR, then the code (asm: runtheprog itself; here a jmp.l to it). */
static struct {
    ULONG len;                          /* dc.l 0 ;length */
    ULONG next;                         /* procstart: dc.l 0 ;next */
    UWORD jmp;                          /* jmp abs.l */
    void (*addr)(void);
} __attribute__((aligned(4))) procstart = { 0, 0, 0x4ef9, runtheprog };

/* blitz2.s:1595 menu0 - compile/run (falls into menu10) */
void menu0(void)
{
    makeexec = 0;
    closeed = 0;
    nocli = 0;
    compile();
    menu10();                           /* RUN! */
}

/* blitz2.s:1605 menu10 - run */
void menu10(void)
{
    if (anyerrs)
        return;                         /* dontrun */
    if (closeed)
        ed_screenclose();               /* close editor */
    else
        openstopreq(executing);         /* .noclose */
    /* .nopanic: find current dir... */
    tasklock = CurrentDir(0);
    CurrentDir(tasklock);
    panictask = NULL;
    if (CreateProc((CONST_STRPTR)procname, 0, MKBADDR(&procstart.next),
                   (LONG)stackfuck)) {
        /* OK, task should be running here! see if we get a panic! */
        for (;;) {                      /* .nostop */
            Wait(panicmask);
            if (stopit == 0)
                continue;
            if (stopit > 0)
                break;                  /* ended OK */
            if (!chkrealstop())
                continue;
            if (panictask)
                RemTask(panictask);     /* BUG(orig) (#24): no cleanup at all */
            break;
        }
        /* .notpanic */
        if (!closeed)
            endstop();
    }
    /* .fatal (BUG(orig) (#25): CreateProc failure leaves the stop requester open) */
    firstlocal = NULL;
    firstglob = NULL;
    if (closeed)
        ed_screenopen2();
    /* .noopen */
    ActivateWindow((struct Window *)CD_L(CD_MYWINDOW2));
}

/* blitz2.s:1688 runtheprog - runs as the new process (procstart) */
static __saveds void runtheprog(void)
{
    struct m68kregs r = {{0}};

    panictask = FindTask(NULL);         /* 276(execbase) */
    CurrentDir(tasklock);
    if (!nocli)
        ed_openio();                    /* open hendrix */
    calcvbr();
    L_(vbr + 132) = (ULONG)&tokens;     /* trap #1 vector as mailbox */
    L_(vbr + 136) = (ULONG)firsttype;   /* trap #2 vector as mailbox */
    L_(vbr + 128 + 15 * 4) = (ULONG)directtrap;
    dirmode = 0;
    firstglob = NULL;
    firstlocal = NULL;
    dirpcat = pc;
    flushc();
    /* @1713 */
    r.d[2] = stackfuck;
    r.d[7] = 0x624c745aUL;              /* 'bLtZ' force cli-type run */
    r.a[0] = (ULONG)argreqla8;          /* cli arguement */
    r.a[1] = pcat;
    r.a[6] = (ULONG)SysBase;            /* asm: a6 = exec after calcvbr */
    /* runthing (1718): Run the thing! */
    callregs((APTR)pcat, &r);
    if (!nocli)
        ed_closeio();                   /* close the Hendrix IO Window */
    stopit = 1;                         /* ended OK! */
    Signal(us, panicmask);
}

/* blitz2.s:1734 calcvbr */
ULONG calcvbr(void)
{
    UWORD d0 = SysBase->AttnFlags & 15;
    UWORD d1 = 0;
    ULONG d2 = 0;

    while (d0) {                        /* position of highest CPU bit */
        d1++;
        d0 >>= 1;
    }
    /* BUG(orig) (#42): a 68010 (d1 == 1) is treated as having no VBR */
    if (d1 >= 2)
        d2 = getvbr();                  /* DIFF: Supervisor() instead of
                                           SuperState/movec/UserState */
    vbr = d2;
    return d2;
}

/* blitz2.s:1751 flash - debug: flicker COLOR00 */
void flash(void)
{
    UWORD d0 = 0xffff;

    do {
        *(volatile UWORD *)0xdff180 = d0;
    } while (d0-- != 0);
}

/* blitz2.s:1758 flash2 */
void flash2(void)
{
    UWORD d0 = 0xffff;

    do {
        *(volatile UWORD *)0xdff180 = 15;
    } while (d0-- != 0);
}

/* blitz2.s:1765 flushlibs - ask for 256 MEG! (forces library expunge;
   BUG(orig) (#26): the memory would leak if the allocation ever succeeded) */
void flushlibs(void)
{
    AllocMem(1024 * 1024 * 256, MEMF_PUBLIC);
}

char defname[] = "blitz2:deflibs";      /* 1770 */
ULONG deflibs;                          /* 1772 */
char acidname[] = "blitz2:acidlibs";    /* 1774 */
ULONG acidlibs;                         /* 1776 */

/* blitz2.s:1778 menu8 - reload all libs */
void menu8(void)
{
    sleepon();
    freeblitzlibs();
    makeblitzlibs();
    loadblitzlibs();
    pointeron();
}

/* blitz2.s:1784 checklib - size of file `name`, 0 if it can't be locked */
ULONG checklib(const char *name)
{
    BPTR d7 = Lock((CONST_STRPTR)name, ACCESS_READ);

    if (!d7)
        return 0;
    /* 'deflibs' has been found! - pick up file length
       (BUG(orig) (#27): Examine result not checked) */
    Examine(d7, namebufffib);
    UnLock(d7);
    return (ULONG)namebufffib->fib_Size;
}

/* blitz2.s:1801 makelibs - find all lib dirs in 'libsdir' and make a list
   of 'em.  return FALSE (asm eq) if error */
BOOL makelibs(void)
{
    deflibs = checklib(defname);
    acidlibs = checklib(acidname);
    return makeblitzlibs();             /* falls through */
}

/* blitz2.s:1816 makeblitzlibs */
BOOL makeblitzlibs(void)
{
    struct FileInfoBlock *fib = namebufffib;
    BPTR d7 = Lock((CONST_STRPTR)libsdir, ACCESS_READ);

    if (!d7)
        return FALSE;
    if (!Examine(d7, fib) || fib->fib_DirEntryType < 0) {
        UnLock(d7);                     /* .done2 */
        return FALSE;
    }
    /* OK to do exnext on dir... */
    while (ExNext(d7, fib)) {
        struct libsnode *d6, **a0;

        if (fib->fib_DirEntryType < 0)
            continue;
        if (endininfo(fib->fib_FileName))
            continue;
        d6 = doallocmem(sizeof(struct libsnode), MEMF_PUBLIC);
        /* insert sorted (case insensitive); &libslist acts as a node */
        a0 = &libslist;
        for (;;) {                      /* .insloop */
            struct libsnode *d5 = *a0;
            const UBYTE *a1 = (const UBYTE *)fib->fib_FileName;
            const UBYTE *a2;
            BOOL here = FALSE;

            if (!d5)
                break;
            a2 = (const UBYTE *)d5->name;
            for (;;) {                  /* .comloop */
                UWORD c0 = *a1++, c1;

                if (!c0) {
                    here = TRUE;        /* I'm shorter */
                    break;
                }
                c1 = *a2++;
                if (!c1)
                    break;              /* I'm longer -> .next */
                if (tstalpha(c0))
                    c0 &= 255 - 32;
                if (tstalpha(c1))
                    c1 &= 255 - 32;
                if ((UBYTE)c0 == (UBYTE)c1)
                    continue;
                if ((UBYTE)c0 < (UBYTE)c1)
                    here = TRUE;        /* I'm less than */
                break;
            }
            if (here)
                break;
            a0 = &d5->next;             /* .next */
        }
        /* .here */
        d6->next = *a0;                 /* next of last is next of me */
        *a0 = d6;                       /* I am next of last */
        d6->used = -1;                  /* set 'Used' flag */
        /* BUG(orig) (#28): no length check, name[32] can overflow */
        strcpy(d6->name, (const char *)fib->fib_FileName);
    }
    /* .ok */
    UnLock(d7);
    return TRUE;
}

/* blitz2.s:1898 frlibslist - free up a libs list */
void frlibslist(void)
{
    struct libsnode *a2 = libslist;

    libslist = NULL;
    while (a2) {
        struct libsnode *a1 = a2;

        a2 = a2->next;
        FreeMem(a1, sizeof(struct libsnode));
    }
}

/* ================================================== asmmac checkers
 * the last parameter of the asm token macro is a 'do it yourself'
 * subroutine that allows you to do your own error chking for silly
 * opmodes!  size/ea's have already been created.
 * d1=opcode, d2=src reg, d3=src ea, d4=dest reg, d5=dest ea, d6=size.
 * return ne if no can do!  (noerror/eaerror/szerror/swapem: main.h) */

/* asmmac movemchk */
WORD movemchk(struct asmregs *r)
{
    ULONG d0;
    int i;

    if (r->srcmode == r->destmode)
        return eaerror;
    if (r->srcmode != 15) {
        if (r->destmode != 15)
            return eaerror;
        /* .regdest */
        d0 = 1UL << (r->srcmode & 31);
        if (d0 & 0x813)
            return eaerror;
        r->opcode |= 1 << 10;
        return swapem;
    }
    /* .regsrc */
    d0 = 1UL << (r->destmode & 31);
    if (d0 & 0xe0b)
        return eaerror;
    if (r->destmode != 4)
        return noerror;
    /* -(An): reverse the register mask (lsl d2 / roxr d6, 16 times) */
    for (i = 0; i < 16; i++) {
        UWORD x = r->srcreg >> 15;

        r->srcreg <<= 1;
        r->size = (r->size >> 1) | (x << 15);
    }
    r->srcreg = r->size;
    return noerror;
}

/* asmmac movepchk */
WORD movepchk(struct asmregs *r)
{
    if (r->srcmode == r->destmode)
        return eaerror;
    if (r->srcmode != 0)
        return noerror;
    r->opcode |= 1 << 7;
    return noerror;
}

/* asmmac movechk (move to/from ccr/sr/usp, size bits) */
WORD movechk(struct asmregs *r)
{
    if (r->srcmode >= 12) {
        if (r->destmode >= 12)
            return eaerror;
        if (r->srcmode == 12) {              /* .fromccr */
            r->opcode = 0x42c0;
            goto from;
        }
        if (r->srcmode == 13) {              /* .fromsr */
            r->opcode = 0x40c0;
            goto from;
        }
        if (r->srcmode == 14) {              /* .fromusp: to address reg? */
            if (r->destmode != 1)
                return eaerror;
            r->opcode = 0x4e68 | r->destreg;
            goto usp;
        }
        return eaerror;
    }
    /* .notfrom */
    if (r->destmode < 12) {                   /* .dosize */
        r->opcode = 0x1000;
        if (r->size == 0)
            return noerror;
        r->opcode = 0x3000;
        if (r->size == 4)
            return noerror;
        r->opcode = 0x2000;
        return noerror;
    }
    if (r->srcmode >= 12)
        return eaerror;
    if (r->destmode == 12) {                  /* .toccr */
        r->opcode = 0x44c0;
        goto to;
    }
    if (r->destmode == 13) {                  /* .tosr */
        r->opcode = 0x46c0;
        goto to;
    }
    if (r->destmode == 14) {                  /* .tousp */
        if (r->srcmode != 1)
            return eaerror;
        r->opcode = 0x4e60 | r->srcreg;
        goto usp;
    }
    return eaerror;
usp:
    r->srcreg = 0;
    r->srcmode = 0;
    r->destreg = 0;
    r->destmode = 0;
    return noerror;
from:
    if (r->size != 4)
        return szerror;
    r->srcmode = 0;
    r->srcreg = 0;
    return swapem;
to:
    if (r->size != 4)
        return szerror;
    r->destmode = 0;
    r->destreg = 0;
    return noerror;
}

/* asmmac exgchk */
WORD exgchk(struct asmregs *r)
{
    if (r->srcmode != r->destmode) {
        r->opcode |= 0x88;
        if (r->srcmode == 0)
            return noerror;
        return swapem;
    }
    /* .skip */
    if (r->srcmode != 0)
        r->opcode |= 0x48;
    else
        r->opcode |= 0x40;                  /* .data */
    return noerror;
}

/* asmmac bitchk */
WORD bitchk(struct asmregs *r)
{
    if (r->srcmode == 0) {
        r->srcreg <<= 9;                    /* review: asm shifts d2 itself */
        r->opcode |= r->srcreg;
        return noerror;
    }
    /* .notdn */
    if (r->size >= 8)
        return szerror;
    r->opcode ^= 0x900;
    return noerror;
}

/* asmmac aslchk2 - d0 = opcode of the memory form */
static WORD aslchk2(struct asmregs *r, UWORD d0)
{
    if ((WORD)r->destmode < 0) {              /* .word: memory shift */
        UWORD d2 = r->srcreg, d3 = r->srcmode;

        r->destmode = 0;                      /* dummy d0 */
        if (r->srcmode == 0)
            return eaerror;
        if (r->size != 4)
            return szerror;
        r->opcode &= 0x100;
        r->opcode |= d0;
        if (d3 >= 7) {
            d2 = d3 - 7;
            d3 = 7;
        }
        r->opcode |= d2;
        r->opcode |= d3 << 3;               /* d2/d3 restored (movem) */
        return noerror;
    }
    if (r->destmode != 0)
        return eaerror;
    r->size <<= 4;
    r->opcode |= r->size;
    r->opcode |= r->destreg;
    if (r->srcmode == 11)
        return noerror;
    if (r->srcmode != 0)
        return eaerror;
    r->srcreg <<= 9;                        /* review: asm shifts d2 itself */
    r->opcode |= r->srcreg;
    r->opcode |= 1 << 5;
    return noerror;
}

/* asmmac roxlchk */
WORD roxlchk(struct asmregs *r) { return aslchk2(r, 0xe4c0); }
/* asmmac rolchk */
WORD rolchk(struct asmregs *r) { return aslchk2(r, 0xe6c0); }
/* asmmac lslchk */
WORD lslchk(struct asmregs *r) { return aslchk2(r, 0xe2c0); }
/* asmmac aslchk */
WORD aslchk(struct asmregs *r) { return aslchk2(r, 0xe0c0); }

/* asmmac abcdchk (asm returned with Z=1 and d0 untouched on success =
   noerror for asmit) */
WORD abcdchk(struct asmregs *r)
{
    if (r->srcmode != r->destmode)
        return eaerror;
    if (r->srcmode != 0)
        r->opcode |= 1 << 3;
    return noerror;
}

/* asmmac addchk - if dest is dn, ok */
WORD addchk(struct asmregs *r)
{
    if (r->destmode == 0)
        return noerror;                 /* asm: rts with Z=1 */
    if (r->srcmode != 0)
        return eaerror;                 /* src not dn - fuct */
    r->opcode |= 1 << 8;                    /* turn into dn,<ea> */
    return swapem;
}

/* asmmac andichk */
WORD andichk(struct asmregs *r)
{
    if (r->destmode == 12) {                  /* dest ccr? */
        if (r->size != 0)
            return szerror;
        return noerror;
    }
    if (r->destmode != 13)                    /* or sr? */
        return noerror;
    if (r->size != 4)
        return szerror;
    return noerror;
}
