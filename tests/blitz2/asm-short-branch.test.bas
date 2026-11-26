; asm-short-branch - the size suffixes of the inline assembler's branches
; (extension, #1075): Bcc, BRA and BSR with .s or .b are short branches
; (the displacement in the opcode's low byte, 2 bytes), with .w word
; branches (4 bytes); without a suffix, and a Bcc with .l, is a word
; branch as in BB2 2.10; .s is .b on every other instruction. The case
; runs the branches and prints the opcode words and sizes it compiled to
; (asserted in asm-short-branch.test.toml).
; Correct: the lines the toml names; the original refuses the first .s
; (Syntax Error).
DEFTYPE .l
!TestCase{branches}
  ; forward Bcc.s, taken: BNE.s over the MOVEQ #0 ($6602)
  MOVEQ #1,d0
  .f1
  BNE.s f2
  MOVEQ #0,d0
  .f2
  PutReg d0,a
  NPrint "BNE.s fwd ",a," ",Hex$(Peek.w(?f1) AND $FFFF)," ",?f2-?f1
  ; backward Bcc.b, a loop of 3: BNE.b to b1 ($66FA, -6)
  MOVEQ #3,d0
  MOVEQ #0,d1
  .b1
  ADDQ.l #1,d1
  SUBQ.l #1,d0
  .b2
  BNE.b b1
  .b3
  MOVE.l d1,d0
  PutReg d0,a
  NPrint "BNE.b back ",a," ",Hex$(Peek.w(?b2) AND $FFFF)," ",?b3-?b2
  ; BSR.s and BRA.s ($6102, $6004)
  MOVEQ #0,d0
  .r1
  BSR.s ssub
  .r2
  BRA.s r3
  .ssub
  MOVEQ #7,d0
  RTS
  .r3
  PutReg d0,a
  NPrint "BSR.s ",a," ",Hex$(Peek.w(?r1) AND $FFFF)," BRA.s ",Hex$(Peek.w(?r2) AND $FFFF)
  ; .w: word branches ($6700 0004, $6000 0002, $6100 xxxx)
  MOVEQ #0,d0
  .w1
  BEQ.w w2
  MOVEQ #1,d0
  .w2
  BRA.w w3
  .w3
  PutReg d0,a
  NPrint "BEQ.w ",a," ",Hex$(Peek.w(?w1) AND $FFFF)," ",Peek.w(?w1+2)," ",?w2-?w1
  NPrint "BRA.w ",Hex$(Peek.w(?w2) AND $FFFF)," ",Peek.w(?w2+2)," ",?w3-?w2
  MOVEQ #0,d0
  .v1
  BSR.w vsub
  .v2
  BRA vend
  .vsub
  MOVEQ #9,d0
  RTS
  .vend
  PutReg d0,a
  NPrint "BSR.w ",a," ",Hex$(Peek.w(?v1) AND $FFFF)," ",Peek.w(?v1+2)," ",?v2-?v1
  ; without a suffix: word branches as in BB2 2.10
  .n1
  BEQ n2
  .n2
  BRA n3
  .n3
  NPrint "BEQ ",Hex$(Peek.w(?n1) AND $FFFF)," ",Peek.w(?n1+2)," ",?n2-?n1," BRA ",?n3-?n2
  .l1
  BEQ.l l2
  .l2
  NPrint "BEQ.l ",Hex$(Peek.w(?l1) AND $FFFF)," ",Peek.w(?l1+2)," ",?l2-?l1
  ; the limits of a short branch: +126 and -128
  BRA skip
  .e1
  BRA.s e2
  Dcb.w 63,$4E71
  .e2
  .e3
  Dcb.w 63,$4E71
  .e4
  BEQ.s e3
  .skip
  NPrint "+126 ",Hex$(Peek.w(?e1) AND $FFFF)," -128 ",Hex$(Peek.w(?e4) AND $FFFF)
  ; .s on another instruction is .b: MOVE.s is MOVE.b ($103C)
  MOVEQ #-1,d0
  .m1
  MOVE.s #5,d0
  PutReg d0,a
  NPrint "MOVE.s ",a," ",Hex$(Peek.w(?m1) AND $FFFF)
!EndTestCase
