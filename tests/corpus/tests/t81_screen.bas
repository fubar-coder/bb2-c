; t81_screen - screenslib: Screen (mode / long form / bitmap), ScreenTags,
; WbToScreen, FindScreen, ViewPort, SMouseX/Y, Show/Hide/Beep/MoveScreen,
; ScreenPens, ShowBitMap, LoadScreen (via SaveScreen), CloseScreen
DEFTYPE .l
Function.l cks{a.l,n.l}
  c.l=0
  For i.l=0 To n-1 Step 4
    c=(c*3+Peek.l(a+i)) AND $FFFFFF
  Next
  Function Return c
End Function
Statement info{n.l}
  s.l=Peek.l(Addr Screen(n))
  Print "scr ",n," open ",s<>0," user ",Hex$(Peek.l(s+342))," pos ",Peek.w(s+8)," ",Peek.w(s+10)
  NPrint " size ",Peek.w(s+12)," ",Peek.w(s+14)
  b.l=s+184
  NPrint "  bm ",Peek.w(b)," ",Peek.w(b+2)," depth ",Peek.b(b+5)," modes ",Hex$(Peek.w(s+76))," flags ",Hex$(Peek.w(s+20))
  NPrint "  title [",Peek$(Peek.l(s+22)),"] font [",Peek$(Peek.l(Peek.l(s+40))),"] ",Peek.w(Peek.l(s+40)+4)
  NPrint "  viewport ",ViewPort(n)-s," rp bm ",Peek.l(s+88)=b," dbuf ",Peek.l(Addr Screen(n)+4)<>0
End Statement
Screen 0,3
info{0}
NPrint "used mouse ",SMouseX=Peek.w(Peek.l(Addr Screen(0))+18)," ",SMouseY=Peek.w(Peek.l(Addr Screen(0))+16)
Screen 1,1,"Mode 1 title"
info{1}
Screen 2,$1A
info{2}
Screen 3,0,0,330,100,2,0,"Long form",1,2
info{3}
Screen 4,4,7,320,64,1,$8000,"Hires 1",0,1
info{4}
CloseScreen 4
NPrint "closed 4 ",Peek.l(Addr Screen(4))
; ScreenPens: pens of the next screen
ScreenPens 1,2,1,3,1,0,2
Screen 4,2,"Pens"
s=Peek.l(Addr Screen(4))
NPrint "pens screen ",s<>0," type ",Hex$(Peek.w(s+20))
ScreenPens 0
Screen 4,2,"No pens"
s=Peek.l(Addr Screen(4))
NPrint "no pens screen ",s<>0," type ",Hex$(Peek.w(s+20))
ScreenPens 1
CloseScreen 4
; front screen / FindScreen / WbToScreen (object 4 is cleared before each
; FindScreen: freescreen would close a found Blitz screen, BUG(orig))
Poke.l Addr Screen(4),0 : FindScreen 4
NPrint "front is 3 ",Peek.l(Addr Screen(4))=Peek.l(Addr Screen(3))
ShowScreen 0
Poke.l Addr Screen(4),0 : FindScreen 4
NPrint "front is 0 ",Peek.l(Addr Screen(4))=Peek.l(Addr Screen(0))
HideScreen 0
Poke.l Addr Screen(4),0 : FindScreen 4
NPrint "front not 0 ",Peek.l(Addr Screen(4))<>Peek.l(Addr Screen(0))
Poke.l Addr Screen(4),0 : FindScreen 4,"Mode 1"
NPrint "find title ",Peek.l(Addr Screen(4))=Peek.l(Addr Screen(1))
Poke.l Addr Screen(4),0 : FindScreen 4,"Long"
NPrint "find long ",Peek.l(Addr Screen(4))=Peek.l(Addr Screen(3))
Poke.l Addr Screen(4),0 : FindScreen 4,""
NPrint "find empty ",Peek.l(Addr Screen(4))<>0
Poke.l Addr Screen(4),0
WbToScreen 4
w=Peek.l(Addr Screen(4))
NPrint "wb ",w<>0," type ",Peek.w(w+20) AND 15," user blitz ",Peek.l(w+342)=$624C745A
CloseScreen 4
NPrint "wb still open ",Peek.l(w+12)>0," object ",Peek.l(Addr Screen(4))
BeepScreen 1
s=Peek.l(Addr Screen(1))
MoveScreen 1,0,30 : NPrint "moved ",Peek.w(s+8)," ",Peek.w(s+10)
MoveScreen 1,0,-30 : NPrint "moved back ",Peek.w(s+8)," ",Peek.w(s+10)
; ScreenTags
r=ScreenTags(4,"Tagged")
s=Peek.l(Addr Screen(4))
NPrint "tags ",r=s," ",s<>0," ",Peek.w(s+12)," ",Peek.w(s+14)," [",Peek$(Peek.l(s+22)),"] ",Hex$(Peek.l(s+342))
r=ScreenTags(4,"Tag list",$80000023,400,$80000024,150,$80000025,2)
s=Peek.l(Addr Screen(4))
NPrint "tags pairs ",r=s," ",Peek.w(s+12)," ",Peek.w(s+14)," ",Peek.b(s+189)," [",Peek$(Peek.l(s+22)),"]"
Dim tg.l(8) : tg(0)=$80000023 : tg(1)=352 : tg(2)=$80000024 : tg(3)=80 : tg(4)=0
r=ScreenTags(4,"Array tags",&tg(0))
s=Peek.l(Addr Screen(4))
NPrint "tags array ",r=s," ",Peek.w(s+12)," ",Peek.w(s+14)
r=ScreenTags(4,"Too deep",$80000025,99)
s=Peek.l(Addr Screen(4))
NPrint "tags deep ",r=s," ",s<>0
; LoadScreen: pattern into screen 0, SaveScreen, load into screen 1 (same size)
CloseScreen 1
Screen 1,3,"Load target"
s0=Peek.l(Addr Screen(0)) : s1=Peek.l(Addr Screen(1))
For p=0 To 2
  pl=Peek.l(s0+192+p*4)
  For i=0 To 399 : Poke.l pl+i*4,i*$01010101+p*$1F3D5B79 : Next
Next
NPrint "pattern ",Hex$(cks{Peek.l(s0+192),1600})," ",Hex$(cks{Peek.l(s0+196),1600})
SaveScreen 0,"RAM:bb2t81.iff"
LoadScreen 1,"RAM:bb2t81.iff"
NPrint "loaded ",Hex$(cks{Peek.l(s1+192),1600})," ",Hex$(cks{Peek.l(s1+196),1600})," ",Hex$(cks{Peek.l(s1+200),1600})
Screen 2,0,0,320,256,3,0,"Load 2",1,2
LoadScreen 2,"RAM:bb2t81.iff",0
NPrint "loaded pal ",Hex$(cks{Peek.l(Peek.l(Addr Screen(2))+192),1600})
KillFile "RAM:bb2t81.iff"
; ShowBitMap (V39+: ChangeVPBitMap) on screen 3, bitmap of the size of the
; screen's bitmap (bytes per row are aligned on AGA)
s=Peek.l(Addr Screen(3))
BitMap 0,Peek.w(s+184)*8,Peek.w(s+186),Peek.b(s+189)
Use Screen 3
ShowBitMap 0
NPrint "showbitmap dbuf ",Peek.l(Addr Screen(3)+4)<>0
ShowBitMap
NPrint "showbitmap own ",Peek.l(Addr Screen(3)+4)<>0
CloseScreen 3
NPrint "closed 3 ",Peek.l(Addr Screen(3))," ",Peek.l(Addr Screen(3)+4)
CloseScreen 0 : CloseScreen 1 : CloseScreen 2 : CloseScreen 4
NPrint "end"
End
