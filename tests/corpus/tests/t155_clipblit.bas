; t155_clipblit - 2dlib ClipBlit in Amiga mode: shapes (GetaShape from a
; pattern bitmap, with handle) blitted with clipping at every edge and
; corner, completely outside, with excess planes (planes on/off) and
; other ClipBlitMode minterms; checksums of the destination planes.
DEFTYPE .l
Statement bsum{bn.l}
  b.l=Addr BitMap(bn)
  For i.l=0 To Peek.b(b+5)-1
    pl.l=Peek.l(b+8+i*4) : s.l=0
    For k.l=0 To Peek.w(b)*Peek.w(b+2)-1 Step 2 : s=(s*5+Peek.w(pl+k))&$FFFFFF : Next
    Print " ",s
  Next
  NPrint ""
End Statement
BitMap 0,64,32,2
b=Addr BitMap(0)
For i=0 To 1
  pl=Peek.l(b+8+i*4)
  For k=0 To 8*32-1 : Poke.b pl+k,(k*7+i*29+k/5) : Next
Next
Use BitMap 0
GetaShape 0,0,0,32,16
GetaShape 1,8,4,20,9
Handle 1,3,2
GetaShape 2,0,0,48,20
BitMap 1,96,48,3
Use BitMap 1
ClipBlit 0,10,8 : ClipBlit 1,50,20 : Print "inside" : bsum{1}
Cls
ClipBlit 0,-10,5 : ClipBlit 0,80,10 : ClipBlit 0,30,-6 : ClipBlit 0,40,40
Print "edges" : bsum{1}
Cls
ClipBlit 0,-20,-10 : ClipBlit 0,85,-12 : ClipBlit 0,-25,38 : ClipBlit 0,90,44
ClipBlit 1,0,0 : ClipBlit 1,95,47
Print "corners" : bsum{1}
Cls
ClipBlit 0,-32,5 : ClipBlit 0,96,5 : ClipBlit 0,5,-16 : ClipBlit 0,5,48
ClipBlit 0,-100,-100 : ClipBlit 0,500,20 : ClipBlit 1,-17,2 : ClipBlit 2,-47,0
Print "outside" : bsum{1}
Cls 7
ClipBlit 0,3,3,0 : ClipBlit 0,40,20,1 : ClipBlit 2,-7,25,2 : ClipBlit 1,70,-3,3
Print "excess" : bsum{1}
Cls
ClipBlitMode $FCA : ClipBlit 0,1,1
ClipBlitMode $FC0 : ClipBlit 0,45,10
ClipBlitMode $F3A : ClipBlit 2,20,25
ClipBlitMode $FCA : ClipBlit 2,-5,-5
Print "modes" : bsum{1}
End
