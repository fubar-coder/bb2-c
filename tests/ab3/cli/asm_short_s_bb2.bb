; BB2 mode: the size suffix .s of an assembler instruction is .b, and a
; branch with .s or .b is a short branch, with .w a word branch, as in
; blitz2 (extension #1075; AB3 mode keeps AmiBlitz3's word branch, case
; asm_short_s): BEQ.s is taken, BRA.s is 2 bytes ($6000+displacement),
; BRA.w 4 bytes, MOVE.s is MOVE.b
MOVEQ #5,d0
CMP.l #5,d0
BEQ.s ok
MOVEQ #0,d0
ok:
PutReg d0,a.l
NPrint a
l1:
BRA.s l2
MOVEQ #0,d0
l2:
BRA.w l3
l3:
NPrint ?l2 - ?l1, " ", Hex$(Peek.w(?l1) AND $FFFF), " ", ?l3 - ?l2
MOVEQ #-1,d0
MOVE.s #5,d0
PutReg d0,a
NPrint a
End
