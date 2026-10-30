; 2dlib-circle-negative-y (#392) - the CPU plot of Circle (cplot)
; rejects y only with a signed compare against the clip height and then
; multiplies the unsigned low word with the bytes per row: a point above
; the top edge (y = -1 = 65535) is written 65535 rows behind the plane.
; Here plane 0 of a 16x100 bitmap (2 bytes per row) is pointed at a zeroed
; block of 140000 bytes, so that rows up to 65535 stay inside it; Circle
; 8,1,3 has points at y = -2 and -1.  Counted: bytes set inside the
; bitmap (rows 0-99) and behind it.
; Correct: points above the top edge are clipped, nothing is written
; behind the bitmap.
; @configs clib
DEFTYPE .l
BitMap 0,16,100,1
o=Addr BitMap(0)
p0=Peek.l(o+8)
m=AllocMem(140000,$10000)
Poke.l o+8,m
Circle 8,1,3,1
Poke.l o+8,p0
n0=0 : n1=0
For i=0 To 199
  If Peek.b(m+i)<>0 Then n0=n0+1
Next
For i=200 To 139999
  If Peek.b(m+i)<>0 Then n1=n1+1
Next
NPrint "inside ",n0>0
NPrint "behind ",n1
FreeMem m,140000
End
