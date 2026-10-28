; t143_colls - collslib in Amiga mode: ShapesHit, ShapeSpriteHit,
; SpritesHit (with handles), RectsHit (edges, negative positions,
; overflow), SetColl/SetCollOdd/SetCollHi (hardware register only; the
; value they read back is not used).  SColl/PColl/DoColl are Blitz mode
; only and not tested.
DEFTYPE .l
BitMap 0,64,16,2
b=Addr BitMap(0)
For i=0 To 127 : Poke.b Peek.l(b+8)+i,i*3 : Poke.b Peek.l(b+12)+i,i*5 : Next
Use BitMap 0
GetaShape 0,0,0,20,10
GetaShape 1,0,0,7,5
Handle 1,2,1
GetaShape 2,0,0,40,6
GetaSprite 0,1
GetaSprite 1,2
a=Addr Sprite(0) : NPrint "sprite 0 ",Peek.w(a+4)," ",Peek.b(a+9)," h ",Peek.w(a+12)," ",Peek.w(a+14)
a=Addr Sprite(1) : NPrint "sprite 1 ",Peek.w(a+4)," ",Peek.b(a+9)," h ",Peek.w(a+12)," ",Peek.w(a+14)
For x=-25 To 25 Step 5
  s$=""
  For y=-12 To 12 Step 4
    s$=s$+Str$(-ShapesHit(0,0,0,1,x,y))
  Next
  NPrint "sh ",x," ",s$
Next
Print "edges ",ShapesHit(0,10,10,0,30,10)," ",ShapesHit(0,10,10,0,29,10)," ",ShapesHit(0,10,10,0,10,20)
NPrint " ",ShapesHit(0,10,10,0,10,19)
Print "edges2 ",ShapesHit(0,30,10,0,10,10)," ",ShapesHit(0,29,10,0,10,10)," ",ShapesHit(0,10,20,0,10,10)
NPrint " ",ShapesHit(0,10,19,0,10,10)
For x=-30 To 30 Step 6
  s$=""
  For y=-12 To 12 Step 4
    s$=s$+Str$(-ShapeSpriteHit(0,0,0,0,x,y))+Str$(-SpritesHit(0,0,0,1,x,y))
  Next
  NPrint "sp ",x," ",s$
Next
Print "rect ",RectsHit(0,0,10,10,10,0,5,5)," ",RectsHit(0,0,10,10,9,0,5,5)," ",RectsHit(0,0,10,10,-5,0,5,5)
NPrint " ",RectsHit(0,0,10,10,-4,0,5,5)
Print "rect2 ",RectsHit(0,0,10,10,0,-5,5,5)," ",RectsHit(0,0,10,10,0,-4,5,5)," ",RectsHit(5,5,0,0,5,5,0,0)
NPrint " ",RectsHit(5,5,1,1,5,5,0,0)
Print "rect3 ",RectsHit(32760,0,10,10,-32768,0,5,5)," ",RectsHit(-32768,0,10,10,32760,0,10,5)
NPrint " ",RectsHit(0,0,-10,-10,-5,-5,3,3)
SetColl 1,2
SetColl 2,3,1
SetColl 3,3,2
SetCollHi 2
SetCollOdd
NPrint "done"
End
