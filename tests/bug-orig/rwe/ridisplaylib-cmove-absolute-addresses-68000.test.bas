; ridisplaylib-cmove-absolute-addresses-68000 (#889) - InitView reads
; the words of its first four copper moves from the absolute addresses
; $8E, $2C81, $90, $2CC1, $92, $38, $94 and $D0 (the macro cmove has no
; # on its operands; test ridisplaylib-cmove-absolute-addresses).  $2C81
; and $2CC1 are odd: on a 68000 the word read is an address error, every
; InitView crashes (the session ends in a timeout).  The machine is the
; A600 (68000).
; Correct: InitView works on a 68000 and writes the register moves
; $008E,$2C81, $0090,$2CC1, $0092,$0038, $0094,$00D0.
DEFTYPE .l

.moves
  Data.l $8E,$2C81,$90,$2CC1,$92,$38,$94,$D0

!TestCase{moves_on_68000}
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
