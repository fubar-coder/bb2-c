; ridisplaylib-cmove-absolute-addresses (#889) - InitView writes the
; first four moves of the copper list with the macro cmove, whose
; operands have no #: !cmove{diwstrt,$2c81} is MOVE.w $8E,(a0)+ /
; MOVE.w $2C81,(a0)+.  The list gets the words stored at $8E, $2C81,
; $90, $2CC1, $92, $38, $94 and $D0 (the vector table and low chip
; memory; on a 68000 the odd ones are an address error, test
; ridisplaylib-cmove-absolute-addresses-68000).  The test counts the
; words that are the register move they should be (what lies at those
; addresses differs between machines).
; Correct: the list begins with $008E,$2C81, $0090,$2CC1, $0092,$0038,
; $0094,$00D0 (DIWSTRT, DIWSTOP, DDFSTRT, DDFSTOP).
DEFTYPE .l

.moves
  Data.l $8E,$2C81,$90,$2CC1,$92,$38,$94,$D0

!TestCase{first_moves}
  c=InitView(256,5,8,32)
  !Check{c<>0,"list"}
  Restore moves
  k=0
  For i=0 To 7
    Read m
    If (Peek.w(c+i*2) AND $FFFF)=m Then k+1
  Next
  !CheckEq{k,8,"moves_right"}
!EndTestCase
