; compiler-copyconst-dest (#997) - a destination operand text of 134
; characters from a macro.
; copyconst copies the text of an assembler operand behind a flag word
; into namebuff (source operand, Dc) or namebuff2 (destination); namebuff2
; follows namebuff, 8 bytes follow namebuff2, then optstuff.
; With the flag word and the 0 the text needs 137 bytes, one more than
; namebuff2 and the 8 bytes hold: blitz2.s wrote its 0 into optstuff
; (longer texts overwrite more of the data behind it).
; Correct: "Operand too Long" at the macro's line.
; @fixed #997
Macro d
move.l #7,destss+`1+`1
End Macro
Goto skip
.destss
Dc.l 0
.skip
!d{0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0}
NPrint Peek.l(?destss)
End
