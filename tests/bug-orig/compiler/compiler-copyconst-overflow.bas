; compiler-copyconst-overflow - copyconst copies the text of an assembler
; operand into namebuff (source operand) or namebuff2 (destination, 128
; bytes behind namebuff) without a length check.  The source operand's
; text here has 127 characters (in a macro the text is copied): its last
; character and its 0 run into namebuff2, and the destination's text
; overwrites them - the source expression is cut and followed by the
; destination's flag word and text.
; Correct: the program compiles, the move stores 64.
Macro m
move.l #`1+`1,dat
End Macro
Goto skip
.dat
Dc.l 0
.skip
!m{1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1}
NPrint Peek.l(?dat)
End
