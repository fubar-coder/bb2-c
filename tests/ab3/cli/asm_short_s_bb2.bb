; BB2 mode: the size suffix .s of an assembler instruction is a Syntax
; Error, as in BB2 2.10 (AB3 mode takes it as .b, case asm_short_s)
MOVEQ #5,d0
CMP.l #5,d0
BEQ.s ok
MOVEQ #0,d0
ok:
a.l = PutD0
NPrint a
End
