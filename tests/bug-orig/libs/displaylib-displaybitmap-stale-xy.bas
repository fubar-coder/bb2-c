; displaylib-displaybitmap-stale-xy (#411) - DisplayBitMap CopList#,BitMap#
; (without x,y) gets only d0/d1 from the compiler but stores d2/d3 in
; the start positions x0/x1 of the CopList (+62/+66), which DisplayScroll
; starts from.  Amiga mode (the list is built in memory only, as in
; t164_display): a CopList without smooth scrolling, d2/d3 loaded with
; known values by inline assembler right before the call.
; Correct: the form without x,y starts at 0,0 (as the double form
; without coordinates and the smooth scrolling resolutions do).
; @configs clib
DEFTYPE .l
BitMap 0,320,32,4
InitCopList 1,44,20,$10003,8,16,-8
c=Addr CopList(1)
MOVE.l #$12345678,d2
MOVE.l #$9ABCDEF0,d3
DisplayBitMap 1,0
NPrint "x0 ",Hex$(Peek.l(c+62))," x1 ",Hex$(Peek.l(c+66))
End
