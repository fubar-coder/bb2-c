; polylib-renderpoly-a3 (#420) - RenderPoly has no !libs and jumps through
; polyvector, which starts as _bitmappoly: before any RenderToBitmap or
; RenderToChunky it takes a3 as the bitmap object, called from BASIC the
; write pointer of the string work buffer.
; Here a3 is set (inline assembler) to a bitmap object built in a block of
; its own (40 bytes per row, 200 rows, depth 1, clip 320x200, plane 0 = a
; zeroed CHIP block), so that the drawing stays inside that block; then
; RenderPoly with a triangle in a short BLITZ ... AMIGA section (as
; t156_bmpoly).  Counted: bytes set in the block's plane.
; Correct: without a RenderTo... RenderPoly draws nowhere through a3.
; @configs clib
DEFTYPE .l
Dim p.w(20)
p(0)=10 : p(1)=10 : p(2)=60 : p(3)=15 : p(4)=40 : p(5)=50
pl=AllocMem(8000,$10002)
f=AllocMem(64,$10001)
Poke.w f,40 : Poke.w f+2,200 : Poke.w f+4,1 : Poke.l f+8,pl
Poke.w f+56,320 : Poke.w f+58,200
a=&p(0)
VWait
BLITZ
GetReg d0,f : MOVE.l d0,a3
RenderPoly 3,a,1
VWait 2
AMIGA
n=0
For i=0 To 7999
  If Peek.b(pl+i)<>0 Then n=n+1
Next
NPrint "drawn through a3 ",n
FreeMem f,64 : FreeMem pl,8000
End
