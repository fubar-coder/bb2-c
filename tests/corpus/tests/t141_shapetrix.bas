; t141_shapetrix - shapetrixlib in Amiga mode: XFlip (blitter shift +
; byte mirror), YFlip, Rotate (several angles), Scale (up, down, mixed,
; with and without a palette); shapes with and without a cookie.
; XFlip only for shape 1 (32 pixels = byte width * 8: no shift blit).
; Shapes that need the shift blit are not tested: the original flips on
; the CPU while the last blit may still run (BUG(orig)); the result
; depends on timing (CPU speed, debugger traps between the statements).
; Results: the shape blocks (without pointers) and data/cookie checksums.
DEFTYPE .l
Function.l cks{a.l,n.l}
  c.l=0
  If a=0 Then Function Return -1
  For i.l=0 To n-1
    c=(c*3+(Peek.b(a+i) AND 255)) AND $FFFFFF
  Next
  Function Return c
End Function
Statement shinfo{t$,s.l}
  a.l=Addr Shape(s)
  Print t$," ",Peek.w(a)," ",Peek.w(a+2)," ",Peek.w(a+4)," ",Peek.w(a+6)," ",Hex$(Peek.w(a+8) AND $FFFF)
  Print " h ",Peek.w(a+10)," ",Peek.w(a+12)," sz ",Peek.w(a+22)," ",Peek.w(a+24)," ",Peek.w(a+26)," ",Peek.w(a+28)
  NPrint " data ",Hex$(cks{Peek.l(a+14),Peek.w(a+26) AND $FFFF})," cookie ",Hex$(cks{Peek.l(a+18),Peek.w(a+22) AND $FFFF})
End Statement
Statement mkshapes{}
  Use BitMap 0
  GetaShape 0,2,1,21,9
  GetaShape 1,0,0,32,6
  AutoCookie Off
  GetaShape 2,5,3,13,11
  AutoCookie On
End Statement
BitMap 0,64,32,3
b=Addr BitMap(0)
For p=0 To 2
  For i=0 To 255
    v=(i*11+p*29+(i/8)*3) AND 255
    If ((i/8) AND 3)=p Then v=v OR $81
    Poke.b Peek.l(b+8+p*4)+i,v
  Next
Next
mkshapes{}
For s=0 To 2 : shinfo{"start "+Str$(s),s} : Next
XFlip 1 : shinfo{"xflip 1",1}
YFlip 0 : shinfo{"yflip 0",0}
mkshapes{}
Rotate 0,0.25 : shinfo{"rot 0.25",0}
mkshapes{}
Rotate 0,0.125 : shinfo{"rot 0.125",0}
mkshapes{}
Rotate 0,0.5 : shinfo{"rot 0.5",0}
mkshapes{}
Rotate 0,0 : shinfo{"rot 0",0}
mkshapes{}
Rotate 0,0.9 : shinfo{"rot 0.9",0}
Rotate 2,0.3 : shinfo{"rot 2 0.3",2}
mkshapes{}
Scale 0,2,2 : shinfo{"scale 2,2",0}
mkshapes{}
Scale 0,0.5,0.5 : shinfo{"scale .5",0}
mkshapes{}
Scale 1,1.5,0.75 : shinfo{"scale 1.5,.75",1}
Scale 2,0.1,0.1 : shinfo{"scale tiny",2}
InitPalette 0,8
For c=0 To 7 : PalRGB 0,c,c*2,15-c*2,c : Next
mkshapes{}
Scale 0,1.25,1.5,0 : shinfo{"scale pal",0}
mkshapes{}
Scale 1,0.6,1.3 : shinfo{"scale again",1}
End
