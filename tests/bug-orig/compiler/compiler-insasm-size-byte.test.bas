; compiler-insasm-size-byte (#1) - insasm stores the size of the node of an
; assembler expression (16 + length of its text) in a byte; freeasms frees
; the node with it. An expression text of 240 characters or more (here a
; Dc.l in a macro, whose text is copied into the node: 4 x 61 + 3 = 247
; characters) gives a wrong size: FreeMem of too few bytes, inject track
; (the compiler runs through inject) reports it.
; Correct: the node is freed with the size it was allocated with.
Macro m
Dc.l `1+`1+`1+`1
End Macro

!TestCase{long_expression}
  Goto skip
  .dat
  !m{1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1}
  .skip
  !CheckEq{Peek.l(?dat),124,"value"}
!EndTestCase
