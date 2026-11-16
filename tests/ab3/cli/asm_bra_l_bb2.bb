; BB2 mode: BRA and BSR take .s, .b and .w (#1075), but not .l (a 32-bit
; displacement is 68020 only): "Illegal Assembler Instruction Size"
BRA.l e1
e1:
End
