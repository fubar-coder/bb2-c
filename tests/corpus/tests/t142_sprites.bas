; t142_sprites - spriteslib in Amiga mode: GetaSprite from 4 and 16
; colour shapes in SpriteMode 0/1/2 (16/32/64 pixels), handles,
; SaveSprites/LoadSprites (both forms).  Shape widths are multiples of
; the sprite width (GetaSprite reads whole sprite widths).  Results: the
; sprite blocks (without the data pointer) and data checksums; files
; parsed block by block without the pointers.
DEFTYPE .l
Function.l cks{a.l,n.l}
  c.l=0
  If a=0 Then Function Return -1
  For i.l=0 To n-1
    c=(c*3+(Peek.b(a+i) AND 255)) AND $FFFFFF
  Next
  Function Return c
End Function
Statement spinfo{s.l,sh.l}
  a.l=Addr Sprite(s)
  n.l=(Peek.w(a+4) LSL sh)*Peek.w(a+6)
  Print "sprite ",s," ",Peek.w(a+4)," ",Peek.w(a+6)," ",Hex$(Peek.w(a+8) AND $FFFF)," ",Peek.w(a+10)
  NPrint " h ",Peek.w(a+12)," ",Peek.w(a+14)," data ",Hex$(cks{Peek.l(a),n})
End Statement
Statement fsum{f$,sh.l}
  If ReadFile(0,f$)
    l.l=Lof(0)
    m.l=AllocMem(l+8,65536)
    ReadMem 0,m,l
    CloseFile 0
    c.l=0 : p.l=0 : k.l=0
    While p<l
      For i.l=4 To 15 : c=(c*3+(Peek.b(m+p+i) AND 255)) AND $FFFFFF : Next
      d.l=Peek.l(m+p) : z.l=(Peek.w(m+p+4) LSL sh)*Peek.w(m+p+6)
      p=p+16 : k=k+1
      If d<>0
        For i=0 To z-1 : c=(c*3+(Peek.b(m+p+i) AND 255)) AND $FFFFFF : Next
        p=p+z
      EndIf
    Wend
    NPrint f$," len ",l," blocks ",k," cks ",Hex$(c)
    FreeMem m,l+8
  EndIf
End Statement
BitMap 0,64,16,2
BitMap 1,64,16,4
For bm=0 To 1
  b=Addr BitMap(bm)
  For p=0 To Peek.b(b+5)-1
    For i=0 To 127
      v=(i*13+p*37+bm*71+(i/8)*9) AND 255
      Poke.b Peek.l(b+8+p*4)+i,v
    Next
  Next
Next
Use BitMap 0
GetaShape 0,0,0,20,8
Handle 0,3,2
GetaShape 1,0,1,40,6
GetaShape 2,0,0,64,9
Use BitMap 1
GetaShape 3,0,0,16,5
GetaShape 4,0,2,32,7
GetaShape 5,0,0,64,4
SpriteMode 0
GetaSprite 0,0 : spinfo{0,2}
GetaSprite 1,1 : spinfo{1,2}
GetaSprite 2,3 : spinfo{2,2}
GetaSprite 3,4 : spinfo{3,2}
SaveSprites 0,3,"RAM:t142a.spr"
fsum{"RAM:t142a.spr",2}
LoadSprites 10,13,"RAM:t142a.spr"
For s=10 To 13 : spinfo{s,2} : Next
LoadSprites 15,"RAM:t142a.spr"
For s=15 To 18 : spinfo{s,2} : Next
SpriteMode 1
GetaSprite 5,2 : spinfo{5,3}
GetaSprite 6,4 : spinfo{6,3}
SaveSprites 5,6,"RAM:t142b.spr"
fsum{"RAM:t142b.spr",3}
SpriteMode 2
GetaSprite 7,2 : spinfo{7,4}
GetaSprite 8,5 : spinfo{8,4}
SpriteMode 0
KillFile "RAM:t142a.spr" : KillFile "RAM:t142b.spr"
End
