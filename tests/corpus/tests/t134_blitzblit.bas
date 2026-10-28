; t134_blitzblit - the Blitz mode variants of blitlib, bblitlib,
; qblitlib and stencillib in a short BLITZ ... AMIGA section (no display,
; no waiting for events, no memory allocation: every blit ends, VWait
; before AMIGA); the
; checksums are taken back in Amiga mode
DEFTYPE .l
BitMap 0,64,32,2
bn=0 : Gosub fill
Use BitMap 0
GetaShape 0,0,0,32,16
GetaShape 1,8,4,20,9
Handle 1,3,2
BitMap 1,96,48,3
bn=1 : Gosub fill
CopyBitMap 1,2
Buffer 0,4000
Queue 0,10
BitMap 3,96,48,1
p=Peek.l(Addr BitMap(3)+8)
For k=0 To 12*48-1 Step 3 : Poke.b p+k,$F0 : Next
Stencil 1,3
Stencil 0,1
Use BitMap 1
BLITZ
Blit 0,5,3 : Blit 1,40,20 : Blit 0,16,10,1
Block 1,40,30
c0=BlitColl(0,5,3)
BBlit 0,0,48,8 : BBlit 0,1,30,30,2
QBlit 0,0,8,28 : QBlit 0,1,50,30
SBlit 0,0,20,20
VWait 2
AMIGA
NPrint "coll ",c0
bn=1 : Gosub bsum
sk=Addr Stencil(0) : pl=Peek.l(sk+4) : t=0
For k=0 To Peek.w(sk)*Peek.w(sk+2)-1 Step 2 : t=(t*5+Peek.w(pl+k))&$FFFFFF : Next
NPrint "stencil ",t
BLITZ
UnQueue 0,2
ShowStencil 0,1
UnBuffer 0
VWait 2
AMIGA
Gosub bsum
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
