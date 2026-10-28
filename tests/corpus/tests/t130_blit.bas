; t130_blit - blitlib in Amiga mode: Blit (with handle, excess planes),
; BlitMode/CookieMode/EraseMode/InvMode/SolidMode, Block, BlitColl.
; Shapes from GetaShape (original shapeslib) on a poked bitmap; results
; as checksums of the destination bitmap planes.  All coordinates keep
; the blits (shape width + extra word) inside the bitmap.
DEFTYPE .l
BitMap 0,64,32,2
bn=0 : Gosub fill
Use BitMap 0
GetaShape 0,0,0,32,16
GetaShape 1,8,4,20,9
Handle 1,3,2
For s=0 To 1
  a=Addr Shape(s)
  Print "shape ",s," ",Peek.w(a)," ",Peek.w(a+2)," ",Peek.w(a+4)," ",Peek.w(a+6)
  NPrint " ",Hex$(Peek.w(a+8))," ",Peek.w(a+10)," ",Peek.w(a+12)," ",Peek.l(a+18)<>0
Next
NPrint "modes ",Hex$(CookieMode)," ",Hex$(EraseMode)," ",Hex$(InvMode)," ",Hex$(SolidMode)
BitMap 1,96,48,3
Use BitMap 1
Blit 0,5,3 : bn=1 : Gosub bsum
Blit 1,40,20 : Gosub bsum
Blit 0,48,30 : Gosub bsum
Blit 0,16,10,1 : Gosub bsum
Blit 1,30,30,0 : Gosub bsum
BlitMode EraseMode : Blit 0,5,3 : Gosub bsum
BlitMode InvMode : Blit 1,40,20 : Gosub bsum
BlitMode SolidMode : Blit 1,50,5 : Gosub bsum
BlitMode CookieMode
Block 0,16,0 : Gosub bsum
Block 1,40,30 : Gosub bsum
NPrint "coll ",BlitColl(0,5,3)," ",BlitColl(1,40,20)," ",BlitColl(0,48,30)
BitMap 2,96,48,3
Use BitMap 2
NPrint "coll empty ",BlitColl(0,10,10)," ",BlitColl(1,40,20)
Blit 1,40,20
NPrint "coll after ",BlitColl(1,40,20)," ",BlitColl(1,70,5)
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
