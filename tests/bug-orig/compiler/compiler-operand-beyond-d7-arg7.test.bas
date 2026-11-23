; compiler-operand-beyond-d7-arg7 (#1185) - maineval (blitz2.s:8860)
; evaluates every argument of a statement or function from the 7th on in
; d6, so an operator there whose right operand binds tighter (a+b*c) has
; its left operand in d7 and the right one beyond the registers: b*c is
; computed into d7 over a, and the addition takes "d8", the register field
; of a0.  RectsHit gets a wrong width and gives 0 instead of -1.  The
; same expression as b*c+a or a+(b*c) is right.
; Correct: the compile error "Expression too Complex" at the line, no
; wrong code.
!TestCase{arg7}
  a.l=1 : b.l=5 : c.l=6
  h1.l=RectsHit(20,0,10,10,0,0,b*c+a,10)
  !CheckEq{h1,-1,"hit_bc_plus_a"}
  h2.l=RectsHit(20,0,10,10,0,0,a+b*c,10)
  !CheckEq{h2,0,"hit_a_plus_bc"}
!EndTestCase
