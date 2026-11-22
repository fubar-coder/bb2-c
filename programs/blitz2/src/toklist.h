/*
 * toklist.h - the compiler's own token list (blitz2.s 283-722 `tokens`,
 * asmmac), one entry per node in source order, as an X-macro list: the
 * includer defines the entry macros and gets the struct members or the
 * initialisers of a token image (tokimg.h; main.c: `tokimg`, and in
 * blitz2-ab3 also ab3tok.c: `tokimg_ab3`), or the token codes (tokcode.h:
 * TOK_sym).  No include guard: included once per use.
 *
 *   TK(id, next, num, sym, name, help)    a token node: next.l num.w name\0 help\0
 *   TA(id, next, num, sym, name, asment)  an asm mnemonic node, its struct asment
 *                                         (the remaining arguments) at the next
 *                                         even address (makeasmtab relies on that)
 *   TKLAST(id, num, sym, name, help)      the last node of blitz2's list
 *   TK3(id, next, num, sym, name, help)   a node only AmiBlitz3's list has, after
 *   TK3LAST(id, num, sym, name, help)     blitz2's last one; the last one of all
 *   M(bb2, ab3)                           a name, help or asm datum that
 *                                         differs: blitz2's, AmiBlitz3's
 *
 * sym is the name of the token's code in tokcode.h (TOK_sym, an asm
 * mnemonic TOK_ASM_sym): blitz2's name in upper case, AmiBlitz3's for the
 * two `Jimi` slots (SYNTAX, EXTB).
 *
 * AmiBlitz3's list (AmiBlitz/AmiBlitz3@58a11658,
 * Sourcecodes/Amiblitz3/Compiler/internal_commands.ab3, included by
 * AmiBlitz3.ab3 at `.internal_commands`) has the same 210 nodes at the same
 * numbers with six other names (`SYNTAX` and `EXTB` in the two `Jimi`
 * slots, `Chip` for MOVEP, `BSS` for TAS, `RunErrsOn`/`RunErrsOff`), and 8
 * more tokens (tnum+33..tnum+40, $80D4-$80DB) at the end: 218.  Its help
 * texts are taken only where the name differs or is new; its asm data
 * differs only for BRA/BSR (dest shift 0, size 0: every size suffix is
 * taken, and the branch stays a word branch), and AB3 mode has it: so
 * BRA.s, the size suffix .s of AB3 mode (#1068), assembles as in
 * AmiBlitz3 v3.14.0.  blitz2's BRA/BSR take .b and .w (asmmac: no size;
 * extension #1075: .b/.s is a short branch, .w a word branch).
 */

TK(tok1, tok2, 1, NEWTYPE, "NEWTYPE", ".Typename")
TK(tok2, tok3, 2, END, "End", "[If|While|Select|Statement|Function|SetInt|SetErr|NEWTYPE|MACRO]")
TK(tok3, tok4, 3, LET, "Let", "Var=|Operator Expression")
TK(tok4, tok5, 4, DIM, "Dim", "Arrayname [List] (Dimension1[,Dimension2..])")
TK(tok5, tok6, 5, GOTO, "Goto", "Program Label")
TK(tok6, tok7, 6, GOSUB, "Gosub", "Program Label")
TK(tok7, tok8, 7, RETURN, "Return", "")
TK(tok8, tok9, 8, STATEMENT, "Statement", "Procedurename{[Parameter1[,Parameter2...]]}")
TK(tok9, tok10, 9, FUNCTION, "Function", "[.Type] Procedurename{[Parameter1[,Parameter2...]]}")
TK(tok10, tok11, 10, MOUSEWAIT, "MouseWait", "")
TK(tok11, tok12, 11, IF, "If", "Expression [Then...]")
TK(tok12, tok13, 12, WHILE, "While", "Expression")
TK(tok13, tok14, 13, MACRO, "Macro", "Macroname")
TK(tok14, tok15, 14, SELECT, "Select", "Expression")
TK(tok15, tok16, 15, CASE, "Case", "Expression")
TK(tok16, tok17, 16, DEFAULT, "Default", "")
TK(tok17, tok18, 17, ASMEXIT, "AsmExit", "")
TK(tok18, tok19, 18, XINCLUDE, "XINCLUDE", "Filename")
TK(tok19, tok20, 19, INCLUDE, "INCLUDE", "Filename")
TK(tok20, tok21, 20, SYNTAX, M("Jimi", "SYNTAX"), M("", "<Mode = Bit0(1) variables need declaration, Bit1(2) a.l=x/a$=x means declaration, Bit2(4) force typecheck>"))
TK(tok21, tok22, 21, SHARED, "SHARED", "Var[,Var...]")
TK(tok22, tok23, 22, FOR, "For", "Var=Expression1 To Expression2 [Step Expression3]")
TK(tok23, tok24, 23, NEXT, "Next", "[Var[,Var...]]")
TK(tok24, tok25, 24, TO, "To", "Expression2 [Step Expression3]")
TK(tok25, tok26, 25, STEP, "Step", "increment")
TK(tok26, tok27, 26, DATA, "Data", "[.Type] Item[,Item...]")
TK(tok27, tok28, 27, READ, "Read", "Var[,Var...]")
TK(tok28, tok29, 28, CNIF, "CNIF", "Constant Comparison Constant")
TK(tok29, tok30, 29, CSIF, "CSIF", "\"String\" Comparison \"String\"")
TK(tok30, tok31, 30, CELSE, "CELSE", "")
TK(tok31, tok32, 31, CEND, "CEND", "")
TK(tok32, tok33, 32, CERR, "CERR", "Errormessage")
TK(tok33, tok34, 33, THEN, "Then", "statement...")
TK(tok34, tok35, 34, ELSE, "Else", "[Statement...]")
TK(tok35, tok36, 35, EVEN4, "Even4", "; align to 4 byte boundary")
TK(tok36, tok37, 36, EVEN8, "Even8", "; align to 8 byte boundary")
TK(tok37, tok38, 37, DC, "Dc", "[.Size] Data[,Data...]")
TK(tok38, tok39, 38, DS, "Ds", "[.Size] Length")
TK(tok39, tok40, 39, EVEN, "Even", "")
TK(tok40, tok41, 40, DCB, "Dcb", "[.Size] Repeats,Data")
TK(tok41, tok42, 41, GETREG, "GetReg", "68000 Reg,Expression")
TK(tok42, tok43, 42, INCBIN, "IncBin", "Filename")
TK(tok43, tok44, 43, FREE, "Free", "Objectname Object#")
TK(tok44, tok45, 44, USE, "Use", "Objectname Object#")
TK(tok45, tok46, 45, STOP, "Stop", "")
TK(tok46, tok47, 46, CONT, "Cont", "[N]")
TK(tok47, tok48, 47, SIZEOF, "SizeOf", ".Typename[,Entrypath]")
TK(tok48, tok49, 48, SETINT, "SetInt", "Type")
TK(tok49, tok50, 49, CLRINT, "ClrInt", "Type")
TK(tok50, tok51, 50, MAXLEN, "MaxLen", "StringVar=Expression")
TK(tok51, tok52, 51, DEFTYPE, "DEFTYPE", ".Typename [Var[,Var...]]")
TK(tok52, tok53, 52, BLITZ, "BLITZ", "")
TK(tok53, tok54, 53, AMIGA, "AMIGA", "")
TK(tok54, tok55, 54, QAMIGA, "QAMIGA", "")
TK(tok55, tok56, 55, VWAIT, "VWait", "[Frames]")
TK(tok56, tok57, 56, ALIBJSR, "ALibJsr", "Token[,Form]")
TK(tok57, tok58, 57, TOKEJSR, "TokeJsr", "Token[,Form]")
TK(tok58, tok59, 58, BLIBJSR, "BLibJsr", "Token[,Form]")
TK(tok59, opabcd, 59, EXTB, M("Jimi", "EXTB"), "")
TA(opabcd, opadd, fnum+0, ABCD, "ABCD",
   0xc100, dn_^pan_, dn_^pan_, (9<<4)+0, (0<<4)+0, (UWORD)((15<<12)+byte_), abcdchk, (WORD)0)
TA(opadd, opaddi, fnum+1, ADD, "ADD",
   0xd000, all_, 0x1fd, (0<<4)+3, (9<<4)+15, (UWORD)((6<<12)+sall_), addchk, (WORD)0x8003)
TA(opaddi, opadda, fnum+2, ADDI, "ADDI",
   0x600, imm_, 0x1fd, (15<<4)+15, (0<<4)+3, (UWORD)((6<<12)+sall_), NULL, (WORD)0)
TA(opadda, opaddq, fnum+3, ADDA, "ADDA",
   0xd0c0, all_, 2, (0<<4)+3, (9<<4)+15, (UWORD)((8<<12)+wordlong_), NULL, (WORD)0)
TA(opaddq, opaddx, fnum+4, ADDQ, "ADDQ",
   0x5000, imm_, 0x1ff, (15<<4)+15, (0<<4)+3, (UWORD)((6<<12)+sall_), NULL, (WORD)1)
TA(opaddx, opand, fnum+5, ADDX, "ADDX",
   0xd100, dn_^pan_, dn_^pan_, (0<<4)+15, (9<<4)+15, (UWORD)((6<<12)+sall_), abcdchk, (WORD)0)
TA(opand, opandi, fnum+6, AND, "AND",
   0xc000, all_&nan_, 0x1fd, (0<<4)+3, (9<<4)+15, (UWORD)((6<<12)+sall_), addchk, (WORD)0)
TA(opandi, opasl, fnum+7, ANDI, "ANDI",
   0x200, imm_, 0x1fd^ccr_^sr_^imm_, (15<<4)+15, (0<<4)+3, (UWORD)((6<<12)+sall_), andichk, (WORD)0)
TA(opasl, opasr, fnum+8, ASL, "ASL",
   0xe100, imm_^0x1fd, 1, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+sall_), aslchk, (WORD)1)
TA(opasr, opbcc, fnum+9, ASR, "ASR",
   0xe000, imm_^0x1fd, 1, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+sall_), aslchk, (WORD)1)
TA(opbcc, opbcs, fnum+10, BCC, "BCC",
   0x6400, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2)
TA(opbcs, opbeq, fnum+11, BCS, "BCS",
   0x6500, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2)
TA(opbeq, opbge, fnum+12, BEQ, "BEQ",
   0x6700, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2)
TA(opbge, opbgt, fnum+13, BGE, "BGE",
   0x6C00, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2)
TA(opbgt, opbhi, fnum+14, BGT, "BGT",
   0x6E00, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2)
TA(opbhi, opble, fnum+15, BHI, "BHI",
   0x6200, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2)
TA(opble, opbls, fnum+16, BLE, "BLE",
   0x6F00, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2)
TA(opbls, opblt, fnum+17, BLS, "BLS",
   0x6300, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2)
TA(opblt, opbmi, fnum+18, BLT, "BLT",
   0x6D00, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2)
TA(opbmi, opbne, fnum+19, BMI, "BMI",
   0x6B00, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2)
TA(opbne, opbpl, fnum+20, BNE, "BNE",
   0x6600, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2)
TA(opbpl, opbvc, fnum+21, BPL, "BPL",
   0x6A00, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2)
TA(opbvc, opbvs, fnum+22, BVC, "BVC",
   0x6800, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2)
TA(opbvs, opbchg, fnum+23, BVS, "BVS",
   0x6900, dpc_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+0), NULL, (WORD)2)
TA(opbchg, opbclr, fnum+24, BCHG, "BCHG",
   0x140, imm_^dn_, 0x1fd, (15<<4)+15, (0<<4)+3, (UWORD)((15<<12)+sall_), bitchk, (WORD)0)
TA(opbclr, opbset, fnum+25, BCLR, "BCLR",
   0x180, imm_^dn_, 0x1fd, (15<<4)+15, (0<<4)+3, (UWORD)((15<<12)+sall_), bitchk, (WORD)0)
TA(opbset, opbtst, fnum+26, BSET, "BSET",
   0x1c0, imm_^dn_, 0x1fd, (15<<4)+15, (0<<4)+3, (UWORD)((15<<12)+sall_), bitchk, (WORD)0)
TA(opbtst, opbra, fnum+27, BTST, "BTST",
   0x100, imm_^dn_, 0x1fd, (15<<4)+15, (0<<4)+3, (UWORD)((15<<12)+sall_), bitchk, (WORD)0)
TA(opbra, opbsr, fnum+28, BRA, "BRA",
   0x6000, dpc_, 0, (15<<4)+15, M((15<<4)+15, 0), (UWORD)M((15<<12)+byteword_, 0), NULL, (WORD)2)
TA(opbsr, opchk, fnum+29, BSR, "BSR",
   0x6100, dpc_, 0, (15<<4)+15, M((15<<4)+15, 0), (UWORD)M((15<<12)+byteword_, 0), NULL, (WORD)2)
TA(opchk, opclr, fnum+30, CHK, "CHK",
   0x4180, 0xffd, dn_, (0<<4)+3, (9<<4)+15, (UWORD)((15<<12)+word_), NULL, (WORD)0)
TA(opclr, opcmp, fnum+31, CLR, "CLR",
   0x4200, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((6<<12)+sall_), NULL, (WORD)0)
TA(opcmp, opcmpi, fnum+32, CMP, "CMP",
   0xb000, all_, dn_, (0<<4)+3, (9<<4)+15, (UWORD)((6<<12)+sall_), NULL, (WORD)0x8003)
TA(opcmpi, opcmpa, fnum+33, CMPI, "CMPI",
   0xc00, imm_, 0x1fd, (15<<4)+15, (0<<4)+3, (UWORD)((6<<12)+sall_), NULL, (WORD)0)
TA(opcmpa, opcmpm, fnum+34, CMPA, "CMPA",
   0xb0c0, all_, an_, (0<<4)+3, (9<<4)+15, (UWORD)((8<<12)+wordlong_), NULL, (WORD)0)
TA(opcmpm, opdbcc, fnum+35, CMPM, "CMPM",
   0xb108, anp_, anp_, (0<<4)+15, (9<<4)+15, (UWORD)((6<<12)+sall_), NULL, (WORD)0)
TA(opdbcc, opdbcs, fnum+36, DBCC, "DBCC",
   0x54C8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5)
TA(opdbcs, opdbeq, fnum+37, DBCS, "DBCS",
   0x55C8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5)
TA(opdbeq, opdbge, fnum+38, DBEQ, "DBEQ",
   0x57C8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5)
TA(opdbge, opdbgt, fnum+39, DBGE, "DBGE",
   0x5CC8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5)
TA(opdbgt, opdbhi, fnum+40, DBGT, "DBGT",
   0x5EC8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5)
TA(opdbhi, opdble, fnum+41, DBHI, "DBHI",
   0x52C8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5)
TA(opdble, opdbls, fnum+42, DBLE, "DBLE",
   0x5FC8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5)
TA(opdbls, opdblt, fnum+43, DBLS, "DBLS",
   0x53C8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5)
TA(opdblt, opdbmi, fnum+44, DBLT, "DBLT",
   0x5DC8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5)
TA(opdbmi, opdbne, fnum+45, DBMI, "DBMI",
   0x5BC8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5)
TA(opdbne, opdbpl, fnum+46, DBNE, "DBNE",
   0x56C8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5)
TA(opdbpl, opdbvc, fnum+47, DBPL, "DBPL",
   0x5AC8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5)
TA(opdbvc, opdbvs, fnum+48, DBVC, "DBVC",
   0x58C8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5)
TA(opdbvs, opdbf, fnum+49, DBVS, "DBVS",
   0x59C8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5)
TA(opdbf, opdra, fnum+50, DBF, "DBF",
   0x51c8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5)
TA(opdra, opdbt, fnum+51, DBRA, "DBRA",
   0x51c8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5)
TA(opdbt, opdivs, fnum+52, DBT, "DBT",
   0x50c8, dn_, dpc_, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)5)
TA(opdivs, opdivu, fnum+53, DIVS, "DIVS",
   0x81c0, 0xffd, dn_, (0<<4)+3, (9<<4)+15, (UWORD)((15<<12)+word_), NULL, (WORD)0)
TA(opdivu, opeor, fnum+54, DIVU, "DIVU",
   0x80c0, 0xffd, dn_, (0<<4)+3, (9<<4)+15, (UWORD)((15<<12)+word_), NULL, (WORD)0)
TA(opeor, opeori, fnum+55, EOR, "EOR",
   0xb100, dn_, 0x1fd, (9<<4)+15, (0<<4)+3, (UWORD)((6<<12)+sall_), NULL, (WORD)0x8002)
TA(opeori, opexg, fnum+56, EORI, "EORI",
   0xa00, imm_, 0x1fd^ccr_^sr_^imm_, (15<<4)+15, (0<<4)+3, (UWORD)((6<<12)+sall_), andichk, (WORD)0)
TA(opexg, opext, fnum+57, EXG, "EXG",
   0xc100, dn_^an_, dn_^an_, (9<<4)+15, (0<<4)+15, (UWORD)((15<<12)+long_), exgchk, (WORD)0)
TA(opext, opillegal, fnum+58, EXT, "EXT",
   0x4800, dn_, 0, (0<<4)+15, (15<<4)+15, (UWORD)((6<<12)+0x32f), NULL, (WORD)0)
TA(opillegal, opjmp, fnum+59, ILLEGAL, "ILLEGAL",
   0x4afc, 0, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0)
TA(opjmp, opjsr, fnum+60, JMP, "JMP",
   0x4ec0, 0x7e4, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0)
TA(opjsr, oplea, fnum+61, JSR, "JSR",
   0x4e80, 0x7e4, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0)
TA(oplea, oplink, fnum+62, LEA, "LEA",
   0x41c0, 0x7e4, an_, (0<<4)+3, (9<<4)+15, (UWORD)((15<<12)+long_), NULL, (WORD)0)
TA(oplink, oplsl, fnum+63, LINK, "LINK",
   0x4e50, an_, imm_, (0<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0)
TA(oplsl, oplsr, fnum+64, LSL, "LSL",
   0xe108, imm_^0x1fd, 1, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+sall_), lslchk, (WORD)1)
TA(oplsr, opmove, fnum+65, LSR, "LSR",
   0xe008, imm_^0x1fd, 1, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+sall_), lslchk, (WORD)1)
TA(opmove, opmovep, fnum+66, MOVE, "MOVE",
   0, all_^ccr_^sr_^usp_, 0x1fd^ccr_^sr_^usp_, (0<<4)+3, (9<<4)+6, (UWORD)((15<<12)+sall_), movechk, (WORD)0x8001)
TA(opmovep, opmovea, fnum+67, MOVEP, M("MOVEP", "Chip"),
   0x108, dn_^dan_, dn_^dan_, (9<<4)+15, (0<<4)+15, (UWORD)((6<<12)+0x10f), movepchk, (WORD)0)
TA(opmovea, opmoveq, fnum+68, MOVEA, "MOVEA",
   0x40, all_, an_, (0<<4)+3, (9<<4)+15, (UWORD)((12<<12)+0x23f), NULL, (WORD)0)
TA(opmoveq, opmovem, fnum+69, MOVEQ, "MOVEQ",
   0x7000, imm_, dn_, (15<<4)+15, (9<<4)+15, (UWORD)((15<<12)+long_), NULL, (WORD)3)
TA(opmovem, opmuls, fnum+70, MOVEM, "MOVEM",
   0x4880, mem_^0x7fc, mem_^0x7fc, (0<<4)+3, (0<<4)+3, (UWORD)((6<<12)+0x10f), movemchk, (WORD)0)
TA(opmuls, opmulu, fnum+71, MULS, "MULS",
   0xc1c0, 0xffd, dn_, (0<<4)+3, (9<<4)+15, (UWORD)((15<<12)+word_), NULL, (WORD)0)
TA(opmulu, opnbcd, fnum+72, MULU, "MULU",
   0xc0c0, 0xffd, dn_, (0<<4)+3, (9<<4)+15, (UWORD)((15<<12)+word_), NULL, (WORD)0)
TA(opnbcd, opneg, fnum+73, NBCD, "NBCD",
   0x4800, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0)
TA(opneg, opnegx, fnum+74, NEG, "NEG",
   0x4400, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((6<<12)+sall_), NULL, (WORD)0)
TA(opnegx, opnop, fnum+75, NEGX, "NEGX",
   0x4000, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((6<<12)+sall_), NULL, (WORD)0)
TA(opnop, opnot, fnum+76, NOP, "NOP",
   0x4e71, 0, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0)
TA(opnot, opor, fnum+77, NOT, "NOT",
   0x4600, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((6<<12)+sall_), NULL, (WORD)0)
TA(opor, opori, fnum+78, OR, "OR",
   0x8000, all_&nan_, 0x1fd, (0<<4)+3, (9<<4)+15, (UWORD)((6<<12)+sall_), addchk, (WORD)0x8002)
TA(opori, oppea, fnum+79, ORI, "ORI",
   0, imm_, 0x1fd^ccr_^sr_^imm_, (15<<4)+15, (0<<4)+3, (UWORD)((6<<12)+sall_), andichk, (WORD)0)
TA(oppea, opreset, fnum+80, PEA, "PEA",
   0x4840, 0x7e4, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+long_), NULL, (WORD)0)
TA(opreset, oprol, fnum+81, RESET, "RESET",
   0x4e70, 0, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0)
TA(oprol, opror, fnum+82, ROL, "ROL",
   0xe118, imm_^0x1fd, 1, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+sall_), rolchk, (WORD)1)
TA(opror, oproxl, fnum+83, ROR, "ROR",
   0xe018, imm_^0x1fd, 1, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+sall_), rolchk, (WORD)1)
TA(oproxl, oproxr, fnum+84, ROXL, "ROXL",
   0xe110, imm_^0x1fd, 1, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+sall_), roxlchk, (WORD)1)
TA(oproxr, oprte, fnum+85, ROXR, "ROXR",
   0xe010, imm_^0x1fd, 1, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+sall_), roxlchk, (WORD)1)
TA(oprte, oprtr, fnum+86, RTE, "RTE",
   0x4e73, 0, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0)
TA(oprtr, oprts, fnum+87, RTR, "RTR",
   0x4e77, 0, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0)
TA(oprts, opsbcd, fnum+88, RTS, "RTS",
   0x4e75, 0, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0)
TA(opsbcd, opscc, fnum+89, SBCD, "SBCD",
   0x8100, dn_^pan_, dn_^pan_, (9<<4)+0, (0<<4)+0, (UWORD)((15<<12)+byte_), abcdchk, (WORD)0)
TA(opscc, opscs, fnum+90, SCC, "SCC",
   0x54c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0)
TA(opscs, opseq, fnum+91, SCS, "SCS",
   0x55c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0)
TA(opseq, opsge, fnum+92, SEQ, "SEQ",
   0x57c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0)
TA(opsge, opsgt, fnum+93, SGE, "SGE",
   0x5Cc0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0)
TA(opsgt, opshi, fnum+94, SGT, "SGT",
   0x5Ec0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0)
TA(opshi, opsle, fnum+95, SHI, "SHI",
   0x52c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0)
TA(opsle, opsls, fnum+96, SLE, "SLE",
   0x5Fc0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0)
TA(opsls, opslt, fnum+97, SLS, "SLS",
   0x53c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0)
TA(opslt, opsmi, fnum+98, SLT, "SLT",
   0x5Dc0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0)
TA(opsmi, opsne, fnum+99, SMI, "SMI",
   0x5Bc0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0)
TA(opsne, opspl, fnum+100, SNE, "SNE",
   0x56c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0)
TA(opspl, opsvc, fnum+101, SPL, "SPL",
   0x5Ac0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0)
TA(opsvc, opsvs, fnum+102, SVC, "SVC",
   0x58c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0)
TA(opsvs, opst, fnum+103, SVS, "SVS",
   0x59c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0)
TA(opst, opsf, fnum+104, ST, "ST",
   0x50c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0)
TA(opsf, opstop, fnum+105, SF, "SF",
   0x51c0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0)
TA(opstop, opsub, fnum+106, STOP, "STOP",
   0x4e72, imm_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0)
TA(opsub, opsubi, fnum+107, SUB, "SUB",
   0x9000, all_, 0x1fd, (0<<4)+3, (9<<4)+15, (UWORD)((6<<12)+sall_), addchk, (WORD)0x8003)
TA(opsubi, opsuba, fnum+108, SUBI, "SUBI",
   0x400, imm_, 0x1fd, (15<<4)+15, (0<<4)+3, (UWORD)((6<<12)+sall_), NULL, (WORD)0)
TA(opsuba, opsubq, fnum+109, SUBA, "SUBA",
   0x90c0, all_, 2, (0<<4)+3, (9<<4)+15, (UWORD)((8<<12)+wordlong_), NULL, (WORD)0)
TA(opsubq, opsubx, fnum+110, SUBQ, "SUBQ",
   0x5100, imm_, 0x1ff, (15<<4)+15, (0<<4)+3, (UWORD)((6<<12)+sall_), NULL, (WORD)1)
TA(opsubx, opswap, fnum+111, SUBX, "SUBX",
   0x9100, dn_^pan_, dn_^pan_, (0<<4)+15, (9<<4)+15, (UWORD)((6<<12)+sall_), abcdchk, (WORD)0)
TA(opswap, optas, fnum+112, SWAP, "SWAP",
   0x4840, dn_, 0, (0<<4)+15, (15<<4)+15, (UWORD)((15<<12)+word_), NULL, (WORD)0)
TA(optas, optrap, fnum+113, TAS, M("TAS", "BSS"),
   0x4ac0, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((15<<12)+byte_), NULL, (WORD)0)
TA(optrap, optrapv, fnum+114, TRAP, "TRAP",
   0x4e40, imm_, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)4)
TA(optrapv, optst, fnum+115, TRAPV, "TRAPV",
   0x4e76, 0, 0, (15<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0)
TA(optst, opunlk, fnum+116, TST, "TST",
   0x4a00, 0x1fd, 0, (0<<4)+3, (15<<4)+15, (UWORD)((6<<12)+sall_), NULL, (WORD)0)
TA(opunlk, endasm, fnum+117, UNLK, "UNLK",
   0x4e58, an_, 0, (0<<4)+15, (15<<4)+15, (UWORD)((15<<12)+nosz_), NULL, (WORD)0)
TK(endasm, toktnum, fnum+118, ENDASM, "EndAsm", "")
TK(toktnum, toktnum2, tnum, LIST, "List", "")
TK(toktnum2, toktnum3, tnum+1, SETERR, "SetErr", "")
TK(toktnum3, toktnum5, tnum+2, CLRERR, "ClrErr", "")
TK(toktnum5, toktnum6, tnum+4, ERRFAIL, "ErrFail", "")
TK(toktnum6, toktnum7, tnum+5, ADDR, "Addr", "Objectname(Object#)")
TK(toktnum7, toktnum8, tnum+6, MOD, "MOD", "")
TK(toktnum8, toktnum9, tnum+7, PI, "Pi", "")
TK(toktnum9, toktnum10, tnum+8, REPEAT, "Repeat", "")
TK(toktnum10, toktnum11, tnum+9, UNTIL, "Until", "Expression")
TK(toktnum11, toktnum12, tnum+10, PUTREG, "PutReg", "68000 Reg,Variable")
TK(toktnum12, toktnum13, tnum+11, POP, "Pop", "Gosub|For|Select|If|While|Repeat")
TK(toktnum13, toktnum14, tnum+12, INCDIR, "INCDIR", "Pathname")
TK(toktnum14, toktnum15, tnum+13, ENDIF, "EndIf", "")
TK(toktnum15, toktnum16, tnum+14, WEND, "Wend", "")
TK(toktnum16, toktnum17, tnum+15, SYSJSR, "SysJsr", "Routine")
TK(toktnum17, toktnum18, tnum+16, WBSTARTUP, "WBStartup", "")
TK(toktnum18, toktnum19, tnum+17, MAXIMUM, "Maximum", "Objectname")
TK(toktnum19, toktnum20, tnum+18, ON, "On", "Expression Goto|Gosub Program Label[,Program Label...]")
TK(toktnum20, toktnum21, tnum+19, OFF, "Off", "")
TK(toktnum21, toktnum22, tnum+20, FOREVER, "Forever", "")
TK(toktnum22, toktnum23, tnum+21, RESTORE, "Restore", "[Program Label]")
TK(toktnum23, toktnum24, tnum+22, EXCHANGE, "Exchange", "Var,Var")
TK(toktnum24, toktnum25, tnum+23, USEPATH, "USEPATH", "Pathtext")
TK(toktnum25, toktnum26, tnum+24, CLOSEED, "CloseEd", "")
TK(toktnum26, toktnum27, tnum+25, NOCLI, "NoCli", "")
TK(toktnum27, toktnum28, tnum+26, BITTST, "BitTst", "")
TK(toktnum28, toktnum29, tnum+27, BITSET, "BitSet", "")
TK(toktnum29, toktnum30, tnum+28, BITCLR, "BitClr", "")
TK(toktnum30, toktnum31, tnum+29, BITCHG, "BitChg", "")
TK(toktnum31, toktnum32, tnum+30, USED, "Used", "Objectname(Object#)")
TK(toktnum32, toktnum33, tnum+31, RUNERRSON, M("Runerrson", "RunErrsOn"), "")
TKLAST(toktnum33, tnum+32, RUNERRSOFF, M("Runerrsoff", "RunErrsOff"), "")
/* AmiBlitz3's tokens since Blitz Basic 2 (internal_commands.ab3): known to
   the tokeniser, "Not supported" when compiled (compile.c, eval.c) */
TK3(toktnum34, toktnum35, tnum+33, USELASTPATH, "USELASTPATH", "   # Uses the previous path set with usepath")
TK3(toktnum35, toktnum36, tnum+34, REGLOAD, "RegLoad", "<regs>   # load max. 5 Vars to FPU regs for faster use")
TK3(toktnum36, toktnum37, tnum+35, REGSAVE, "RegSave", "   # store changed loaded regs")
TK3(toktnum37, toktnum38, tnum+36, OBJECT, "Object", "<Name>    # define object #developobject=1 disable it")
TK3(toktnum38, toktnum39, tnum+37, USEPATH2, "USEPATH2", "<Pathtext>   # a second type path can acsess with \\")
TK3(toktnum39, toktnum40, tnum+38, LASTRUNERRSMODE, "LastRunErrsMode", "   # Set the runerror state that was before RunErrsOff")
TK3(toktnum40, toktnum41, tnum+39, KEEP, "KEEP", "")
TK3LAST(toktnum41, tnum+40, OPTIMIZE, "OPTIMIZE", "Mode = [1] Integer on, [2] FPU on, [4] New Syntax Mode, [8] Branches")
