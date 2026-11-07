; compiler-insasm-size-byte (#1) - insasm stores the size of the node of an
; assembler expression (16 + length of its text) in a byte; freeasms frees
; the node with it. An expression text of 240 characters or more (here a
; Dc.l in a macro, whose text is copied into the node: 4 x 61 + 3 = 247
; characters) gives a wrong size: FreeMem of too few bytes, inject track
; reports it.
; Correct: the node is freed with the size it was allocated with.
; @fixed #1
; @pass wrap=@INJECT@|track
Macro m
Dc.l `1+`1+`1+`1
End Macro
Goto skip
.dat
!m{1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1}
.skip
NPrint Peek.l(?dat)
End
