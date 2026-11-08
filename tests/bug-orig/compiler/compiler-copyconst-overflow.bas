; compiler-copyconst-overflow (#997) - copyconst copies the text of an assembler
; operand into namebuff (source operand) or namebuff2 (destination, 128
; bytes behind namebuff); blitz2.s did that without a length check.  The
; source operand's text here has 127 characters (in a macro the text is
; kept): its last character and its 0 lie in namebuff2, and in blitz2.s
; the destination's flag word and text overwrote them - the source
; expression was cut and followed by the destination's text ("Syntax
; Error").  With a destination 125 characters fit
; (compiler-copyconst-limit).
; Correct: "Operand too Long" at the macro's line.
; @fixed #997
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
