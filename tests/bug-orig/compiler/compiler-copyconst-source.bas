; compiler-copyconst-source (#997) - a source operand text of 262
; characters from a macro, with a register as destination.
; copyconst copies the text of an assembler operand behind a flag word
; into namebuff (source operand, Dc) or namebuff2 (destination); namebuff2
; follows namebuff, 8 bytes follow namebuff2, then optstuff.
; With the flag word and the 0 the text needs 265 bytes, one more than
; namebuff, namebuff2 and the 8 bytes hold: blitz2.s wrote its 0 into
; optstuff (longer texts overwrite more of the data behind it).
; Correct: "Operand too Long" at the macro's line.
; @fixed #997
Macro r
move.l #`1+`1+`1+`1+`2,d0
End Macro
!r{1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1,11+1+1+1+1+1+1+1+1+1+1}
NPrint 0
End
