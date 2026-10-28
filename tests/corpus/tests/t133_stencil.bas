; t133_stencil - stencillib in Amiga mode: Stencil (OR of the planes),
; SBlit (with excess planes, cookie ORed into the stencil), SBlitMode;
; checksums of the bitmap and the stencil.  Coordinates keep every blit
; inside the bitmap.
DEFTYPE .l
BitMap 0,64,32,2
bn=0 : Gosub fill
Use BitMap 0
GetaShape 0,0,0,32,16
GetaShape 1,8,4,20,9
Handle 1,3,2
BitMap 1,96,48,3
bn=1 : Gosub fill
p=Peek.l(Addr BitMap(1)+8)
For k=0 To 12*48-1 : Poke.b p+k,0 : Next
p=Peek.l(Addr BitMap(1)+12)
For k=0 To 12*48-1 Step 2 : Poke.b p+k,0 : Next
Use BitMap 1
Stencil 0,1
sk=Addr Stencil(0)
NPrint "stencil ",Peek.w(sk)," ",Peek.w(sk+2)," ",Peek.l(sk+4)<>0
Gosub ssum
SBlit 0,0,10,10 : Gosub bsum : Gosub ssum
SBlit 0,1,40,20,2 : Gosub bsum : Gosub ssum
SBlitMode EraseMode
SBlit 0,0,50,30 : Gosub bsum : Gosub ssum
SBlitMode CookieMode
Stencil 1,0
NPrint "stencil1 ",Peek.w(Addr Stencil(1))," ",Peek.w(Addr Stencil(1)+2)
End

ssum:
  sk=Addr Stencil(0) : pl=Peek.l(sk+4) : t=0
  For k=0 To Peek.w(sk)*Peek.w(sk+2)-1 Step 2 : t=(t*5+Peek.w(pl+k))&$FFFFFF : Next
  NPrint "  stencil sum ",t
Return

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
