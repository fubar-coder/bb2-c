; BB2 mode: a short branch (#1075) whose target lies 128 bytes ahead, one
; beyond the +126 of an 8-bit displacement, is "Illegal Displacement"
BRA.s e2
Dcb.w 64,$4E71
e2:
End
