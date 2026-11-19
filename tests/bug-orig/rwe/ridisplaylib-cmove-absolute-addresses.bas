; ridisplaylib-cmove-absolute-addresses (#889) - InitView writes the
; first four moves of the copper list with the macro cmove, whose
; operands have no #: !cmove{diwstrt,$2c81} is MOVE.w $8E,(a0)+ /
; MOVE.w $2C81,(a0)+.  The list gets the words stored at $8E, $2C81,
; $90, $2CC1, $92, $38, $94 and $D0 (the vector table and low chip
; memory; on a 68000 the odd ones are an address error, test
; ridisplaylib-cmove-absolute-addresses-68000).  The program prints each
; word that is the register move it should be, and "other" for any
; other word (what lies at those addresses differs between machines).
; Correct: the list begins with $008E,$2C81, $0090,$2CC1, $0092,$0038,
; $0094,$00D0 (DIWSTRT, DIWSTOP, DDFSTRT, DDFSTOP).
; @fixed #889
; @rwe ridisplaylib
DEFTYPE .l
c=InitView(256,5,8,32)
If c=0 Then NPrint "no list" : End
Restore moves
For i=0 To 7
  Read m
  w=Peek.w(c+i*2) AND $FFFF
  If w=m Then NPrint "word ",i," ",Hex$(w) Else NPrint "word ",i," other"
Next
End
moves:
Data.l $8E,$2C81,$90,$2CC1,$92,$38,$94,$D0
