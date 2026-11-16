; brexxlib-type-ffff (#102) - Type takes rawkeylib unconvert's result
; unchecked: a character without a raw key code gives a RAWKEY event
; with code $FFFF and qualifier 1 (left shift).
; Here: Type "a"+Chr$(200)+"b" to a tape (QuietTrap On).
; Correct: the character without a code is skipped, the tape holds the
; two events of "a" and "b".  The original has three, the middle one
; with code $FFFF.
; @fixed #102
; @configs clib
DEFTYPE .l
QuietTrap On
TapeTrap 0
Type "a"+Chr$(200)+"b"
e=Peek.l(Addr Tape(0))
While e<>0
  NPrint "class ",Peek.b(e+4)," code ",Right$(Hex$(Peek.w(e+6)),4)," qualifier ",Right$(Hex$(Peek.w(e+8)),4)
  e=Peek.l(e)
Wend
End
