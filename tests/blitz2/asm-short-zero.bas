; asm-short-zero - a short branch (#1075) to the next instruction: the
; displacement 0 cannot be encoded (a 0 in the low byte of a branch makes
; it a word branch), "Illegal Displacement" at the line of the branch
BNE.s e1
e1:
End
