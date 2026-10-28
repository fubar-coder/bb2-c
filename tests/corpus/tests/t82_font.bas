; t82_font - fontlib: BitMapOutput (Print into a bitmap with the BlitzFont),
; Locate, CursX, CursY, Colour, LoadBlitzFont, Use BlitzFont, line wrap,
; bottom edge, character range; intuifontlib: LoadFont (style), Use
; IntuiFont and the font of a new screen.  Results: plane checksums and
; one character as bit pattern.
DEFTYPE .l
Function.l cks{a.l,n.l}
  c.l=0
  For i.l=0 To n-1 Step 4
    c=(c*3+Peek.l(a+i)) AND $FFFFFF
  Next
  Function Return c
End Function
Statement sums{t$}
  b.l=Addr BitMap(0)
  NPrint t$," ",Hex$(cks{Peek.l(b+8),3200})," ",Hex$(cks{Peek.l(b+12),3200})," cur ",Peek.w(b+40)," ",Peek.w(b+42)
End Statement
f=Peek.l(Addr BlitzFont(0))
NPrint "font 0 ",f<>0," y ",Peek.w(f+20)," x ",Peek.w(f+24)," lo ",Peek.b(f+32) AND 255," hi ",Peek.b(f+33) AND 255
BitMap 0,320,80,2
b=Addr BitMap(0)
NPrint "bitmap ",Peek.w(b)," ",Peek.w(b+2)," ",Peek.w(b+4)
BitMapOutput 0
Print "Hello"
DefaultOutput
sums{"hello"}
; the 'H' at x=0, y=0 of plane 0
For y=0 To 7
  v=Peek.b(Peek.l(b+8)+y*40) AND 255
  s$=""
  For k=7 To 0 Step -1
    If v AND (1 LSL k) Then s$=s$+"#" Else s$=s$+"."
  Next
  NPrint "H ",s$
Next
BitMapOutput 0
Locate 10,2 : Print "at 10,2"
DefaultOutput
NPrint "curs ",CursX," ",CursY
sums{"locate"}
BitMapOutput 0
Locate 3,1.5 : DefaultOutput : NPrint "curs q ",CursX," ",CursY
BitMapOutput 0
Colour 3,1 : Locate 0,4 : Print "Col 3/1"
Colour 2 : Print "c2"
NPrint "" : Print "next line"
DefaultOutput
sums{"colour"}
BitMapOutput 0
Colour 1,0 : Locate 36,5 : Print "WRAPPED"
DefaultOutput
sums{"wrap"}
BitMapOutput 0
Locate 0,9.5 : Print "below" : Locate 39,9 : Print "XY"
DefaultOutput
sums{"below"}
BitMapOutput 0
Locate 0,0 : Print Chr$(1)+Chr$(31)+"a"+Chr$(127)+Chr$(160)+Chr$(255)+Chr$(10)+"b"
DefaultOutput
sums{"range"}
LoadBlitzFont 1,"topaz.font"
f1=Peek.l(Addr BlitzFont(1))
NPrint "font 1 ",f1<>0," same as 0 ",f1=f
Use BlitzFont 1
BitMapOutput 0
Locate 20,7 : Print "font1"
DefaultOutput
sums{"font1"}
s$=Str$(1234)
NPrint "str$ still ok [",s$,"]"
; ---------------- intuifontlib
LoadFont 0,"topaz.font",8
a=Addr IntuiFont(0)
NPrint "ifont 0 [",Peek$(Peek.l(a)),"] ",Peek.w(a+4)," ",Peek.b(a+6)," ",Peek.b(a+7)," open ",Peek.l(a+8)<>0
NPrint "  font y ",Peek.w(Peek.l(a+8)+20)
LoadFont 1,"topaz.font",9,1
a=Addr IntuiFont(1)
NPrint "ifont 1 [",Peek$(Peek.l(a)),"] ",Peek.w(a+4)," style ",Peek.b(a+6)," flags ",Peek.b(a+7)," open ",Peek.l(a+8)<>0
NPrint "  font y ",Peek.w(Peek.l(a+8)+20)
LoadFont 1,"topaz.font",9,$201
NPrint "ifont 1 again ",Peek.w(a+4)," style ",Peek.b(a+6)," flags ",Peek.b(a+7)," font y ",Peek.w(Peek.l(a+8)+20)
Screen 0,1,"Font 9"
s=Peek.l(Addr Screen(0))
NPrint "screen font [",Peek$(Peek.l(Peek.l(s+40))),"] ",Peek.w(Peek.l(s+40)+4)
Use IntuiFont 0
Screen 1,1,"Font 8"
s=Peek.l(Addr Screen(1))
NPrint "screen font [",Peek$(Peek.l(Peek.l(s+40))),"] ",Peek.w(Peek.l(s+40)+4)
CloseScreen 0 : CloseScreen 1
NPrint "end"
End
