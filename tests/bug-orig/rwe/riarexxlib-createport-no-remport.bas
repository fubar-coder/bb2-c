; riarexxlib-createport-no-remport (#528) - RXCreatePort makes the port
; public with AddPort and only then allocates the 256-byte block of its
; message lists; if that fails it deletes the port with DeleteMsgPort
; without RemPort: the freed port stays in exec's list of public ports.
; The program runs through inject with that allocation failing
; (runwrap=, inject allocfail: AllocVec of 256 bytes).  It looks for the
; port by walking the list itself, comparing a node's name before it
; looks at its successor (FindPort would stop at the freed node when the
; free list's link written over its ln_Succ is 0); if it is still listed,
; the program unlinks it (the neighbours are found from both ends of the
; list) so the system stays usable.
; Correct: RXCreatePort returns 0 and the port is not listed.
; @rwe riarexxlib
; @pass runwrap=@INJECT@|allocfail=256-264/1
DEFTYPE .l
r=RXCreatePort("BUGORIG")
Forbid_
h=Peek.l(4)+$188
p=0 : b=Peek.l(h)
While b<>0 AND p=0
  nm=Peek.l(b+10)
  If nm<>0 Then If Peek$(nm)="BUGORIG" Then p=b
  b=Peek.l(b)
Wend
If p
  a=h : b=Peek.l(h)
  While b<>p AND Peek.l(b)<>0
    a=b : b=Peek.l(b)
  Wend
  c=h+4 : b=Peek.l(h+8)
  While b<>p AND Peek.l(b+4)<>0
    c=b : b=Peek.l(b+4)
  Wend
  Poke.l a,c
  Poke.l c+4,a
EndIf
Permit_
NPrint "result ",r
NPrint "listed ",p<>0
End
