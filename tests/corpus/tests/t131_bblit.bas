; t131_bblit - bblitlib in Amiga mode: Buffer, BBlit (with excess
; planes), UnBuffer (background back), FlushBuffer, BBlitMode,
; ShowStencil (stencillib Stencil as mask); checksums of the bitmap planes.
; Coordinates keep every blit inside the bitmap, the buffer (4000 bytes)
; holds all records (16 + planes * (4 + shape plane size) each).
DEFTYPE .l
BitMap 0,64,32,2
bn=0 : Gosub fill
Use BitMap 0
GetaShape 0,0,0,32,16
GetaShape 1,8,4,20,9
Handle 1,3,2
BitMap 1,96,48,3
bn=1 : Gosub fill
Use BitMap 1
Gosub bsum
Buffer 0,4000
b0=Addr Buffer(0)
NPrint "buffer ",Peek.l(b0)=Peek.l(b0+8)," ",Peek.l(b0+4)," ",Peek.l(b0+12)
BBlit 0,0,10,10 : Gosub bsum
BBlit 0,1,40,20 : Gosub bsum
BBlit 0,0,20,15,3 : Gosub bsum
NPrint "used ",Peek.l(b0)-Peek.l(b0+8)
UnBuffer 0 : Gosub bsum
NPrint "after ",Peek.l(b0)=Peek.l(b0+8)," ",Peek.l(b0+4)
BBlitMode InvMode
BBlit 0,0,30,5 : BBlit 0,1,50,30 : Gosub bsum
FlushBuffer 0
UnBuffer 0 : Gosub bsum
BBlitMode CookieMode
; ShowStencil: background through the stencil of bitmap 0
BitMap 3,96,48,1
p=Peek.l(Addr BitMap(3)+8)
For k=0 To 12*48-1 Step 3 : Poke.b p+k,$F0 : Next
Stencil 0,3
Use BitMap 1
BBlit 0,0,10,10 : BBlit 0,1,50,20
ShowStencil 0,0 : Gosub bsum
UnBuffer 0 : Gosub bsum
End

; a pattern in every plane of bitmap bn
fill:
  b=Addr BitMap(bn)
  For i=0 To Peek.b(b+5)-1
    pl=Peek.l(b+8+i*4)
    For k=0 To Peek.w(b)*Peek.w(b+2)-1 : Poke.b pl+k,(k*7+i*29+k/5) : Next
  Next
Return

bsum:
  b=Addr BitMap(bn) : t=0
  For i=0 To Peek.b(b+5)-1
    pl=Peek.l(b+8+i*4) : s=0
    For k=0 To Peek.w(b)*Peek.w(b+2)-1 Step 2 : s=(s*5+Peek.w(pl+k))&$FFFFFF : Next
    Print " ",s
  Next
  NPrint ""
Return
