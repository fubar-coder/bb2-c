; compiler-copyconst-local (#997) - a source operand with a local label,
; outside a macro, and a destination operand behind it: copyconst puts the
; name of the last global label (60 characters) in front of the local
; label, the text (126 characters) is kept.
; copyconst copies the text of an assembler operand behind a flag word
; into namebuff (source operand, Dc) or namebuff2 (destination); namebuff2
; follows namebuff, 8 bytes follow namebuff2, then optstuff.
; The 0 of the source text lies in namebuff2, and the destination's flag
; word overwrites it: in blitz2.s the source expression runs on into the
; destination's text.
; Correct: "Operand too Long" at the move.
; @fixed #997
bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb:
move.l #'a+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0,dat
NPrint 0
End
'a
Dc.l 0
dat:
Dc.l 0
