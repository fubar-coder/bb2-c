; t132_qblit - qblitlib in Amiga mode: Queue, QBlit (with excess
; planes), UnQueue (cleared), UnQueue with a bitmap (copied back),
; FlushQueue, QBlitMode; checksums of the bitmap planes.  The queue has
; room for 10 items, the coordinates keep every blit inside the bitmap.
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
Use BitMap 1
Gosub bsum
Queue 0,10
q=Addr Queue(0)
NPrint "queue ",Peek.l(q)=Peek.l(q+4)," ",Peek.l(q+8)
QBlit 0,0,10,10 : Gosub bsum
QBlit 0,1,40,20,1 : Gosub bsum
NPrint "items ",(Peek.l(q)-Peek.l(q+4))/14," ",Peek.w(Peek.l(q+4))," ",Hex$(Peek.w(Peek.l(q+4)+2))," ",Peek.w(Peek.l(q+4)+4)
UnQueue 0 : Gosub bsum
NPrint "empty ",Peek.l(q)=Peek.l(q+4)
QBlitMode SolidMode
QBlit 0,0,24,24 : QBlit 0,1,50,6 : Gosub bsum
UnQueue 0,2 : Gosub bsum
QBlitMode CookieMode
QBlit 0,0,8,8
FlushQueue 0
UnQueue 0 : Gosub bsum
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
