; t100_iff - iffmakelib (SaveBitmap, SaveShape, SavePalette, SaveScreen),
; ilbmifflib (ILBMInfo/Width/Height/Depth/ViewMode, DecodeILBM; ilbminit,
; ilbmgetbyte and ilbmbitmap through LoadShape/LoadBitMap) and iffiolib
; (all reading, LoadPalette).  Files in RAM:, results as checksums.
DEFTYPE .l
Function.l cks{a.l,n.l}
  c.l=0
  For i.l=0 To n-1
    c=(c*3+(Peek.b(a+i) AND 255)) AND $FFFFFF
  Next
  Function Return c
End Function
Statement hexd{a.l,n.l}
  s$=""
  For i.l=0 To n-1
    s$=s$+Right$(Hex$(Peek.b(a+i) AND 255),2)
    If (i AND 31)=31 Then NPrint "  ",s$ : s$=""
  Next
  If s$<>"" Then NPrint "  ",s$
End Statement
Statement fdump{f$,n.l}
  If ReadFile(0,f$)
    l.l=Lof(0)
    m.l=AllocMem(l+8,65536)
    ReadMem 0,m,l
    CloseFile 0
    NPrint f$," len ",l," cks ",Hex$(cks{m,l})
    If n>l Then n=l
    hexd{m,n}
    FreeMem m,l+8
  Else
    NPrint f$," missing"
  EndIf
End Statement
Statement bmsum{t$,n.l}
  b.l=Addr BitMap(n)
  s$=t$+" "+Str$(Peek.w(b))+"x"+Str$(Peek.w(b+2))+"x"+Str$(Peek.b(b+5))
  For p.l=0 To Peek.b(b+5)-1
    s$=s$+" "+Hex$(cks{Peek.l(b+8+p*4),Peek.w(b)*Peek.w(b+2)})
  Next
  NPrint s$
End Statement
Statement palsum{t$,n.l}
  p.l=Addr Palette(n)
  c.l=Peek.l(p) : k.l=Peek.w(p+4)
  NPrint t$," colours ",k
  If c<>0 AND k>0 Then hexd{c+4,k*12}
End Statement
; ---- a bitmap with runs and literals of every length (1280 pixels: 160
; bytes per row, more than 128)
BitMap 0,1280,4,2
b.l=Addr BitMap(0)
seed.l=7
For p.l=0 To 1
  pl.l=Peek.l(b+8+p*4)
  For r.l=0 To 3
    For i.l=0 To 159
      Select r*2+p
      Case 0 : v.l=$55
      Case 1 : v=i
      Case 2 : v=(i/3) AND 1
      Case 3 : If i<130 Then v=$AA Else v=i
      Default
        seed=(seed*75+74) MOD 65537
        v=(seed/7) AND 3
        If (seed AND 15)=0 Then v=$80
      End Select
      Poke.b pl+r*160+i,v
    Next
  Next
Next
; lone byte / lone word at the row end
Poke.b pl+159,$11 : Poke.b pl+158,$12 : Poke.b pl+157,$12
bmsum{"bm0",0}
InitPalette 0,4
PalRGB 0,0,0,0,0 : PalRGB 0,1,15,0,0 : PalRGB 0,2,0,8,15
AGAPalRGB 0,3,255,128,1
palsum{"pal0",0}
SaveBitmap 0,"RAM:t100a.iff"
fdump{"RAM:t100a.iff",48}
SaveBitmap 0,"RAM:t100b.iff",0
fdump{"RAM:t100b.iff",64}
SavePalette 0,"RAM:t100.pal"
fdump{"RAM:t100.pal",32}
InitPalette 1,3
PalRGB 1,0,1,2,3 : PalRGB 1,1,4,5,6 : PalRGB 1,2,7,8,9
SavePalette 1,"RAM:t100o.pal"
fdump{"RAM:t100o.pal",32}
; ---- ILBMInfo
r=ILBMInfo("RAM:t100b.iff")
NPrint "info ",r," ",ILBMWidth," ",ILBMHeight," ",ILBMDepth," ",Hex$(ILBMViewMode)
r=ILBMInfo("RAM:t100.missing")
NPrint "info missing ",r," ",ILBMWidth
r=ILBMInfo("RAM:t100.pal")
NPrint "info pal ",r
BitMap 2,16,300,1
SaveBitmap 2,"RAM:t100l.iff"
r=ILBMInfo("RAM:t100l.iff")
NPrint "info lace ",r," ",ILBMWidth," ",ILBMHeight," ",ILBMDepth," ",Hex$(ILBMViewMode)
; ---- loading back (iffiolib, ilbmifflib ilbmbitmap)
BitMap 1,1280,4,2
LoadBitMap 1,"RAM:t100b.iff",2
bmsum{"loaded",1}
palsum{"pal2",2}
LoadPalette 3,"RAM:t100b.iff"
palsum{"pal3",3}
BitMap 1,640,6,1
LoadBitMap 1,"RAM:t100a.iff"
bmsum{"smaller",1}
; ---- DecodeILBM from memory
If ReadFile(0,"RAM:t100b.iff")
  l=Lof(0) : m=AllocMem(l+8,65536) : ReadMem 0,m,l : CloseFile 0
  BitMap 3,1280,4,2
  DecodeILBM 3,m
  bmsum{"decoded",3}
  FreeMem m,l+8
EndIf
; uncompressed ILBM, two planes, 32 x 2
m=AllocMem(200,65536)
Poke.l m,$464F524D : Poke.l m+4,4+8+20+8+16
Poke.l m+8,$494C424D
Poke.l m+12,$424D4844 : Poke.l m+16,20
Poke.w m+20,32 : Poke.w m+22,2 : Poke.b m+28,2 : Poke.b m+30,0
Poke.l m+40,$424F4459 : Poke.l m+44,16
For i=0 To 15 : Poke.b m+48+i,i*17+1 : Next
BitMap 4,32,2,2
DecodeILBM 4,m
bmsum{"raw",4}
FreeMem m,200
; ---- shapes
Use BitMap 0
GetaShape 0,0,0,48,3
SaveShape 0,"RAM:t100s.iff"
fdump{"RAM:t100s.iff",48}
SaveShape 0,"RAM:t100t.iff",1
fdump{"RAM:t100t.iff",64}
LoadShape 1,"RAM:t100s.iff"
s.l=Addr Shape(1)
NPrint "shape ",ShapeWidth(1)," ",ShapeHeight(1)," ",Peek.w(s+4)," ",Hex$(cks{Peek.l(s+14),Peek.w(s+22)*Peek.w(s+4)})
; ---- a screen
Screen 0,0,0,320,24,2,0,"t100",1,2
SaveScreen 0,"RAM:t100c.iff"
fdump{"RAM:t100c.iff",64}
CloseScreen 0
KillFile "RAM:t100a.iff" : KillFile "RAM:t100b.iff" : KillFile "RAM:t100.pal"
KillFile "RAM:t100o.pal" : KillFile "RAM:t100l.iff" : KillFile "RAM:t100s.iff"
KillFile "RAM:t100t.iff" : KillFile "RAM:t100c.iff"
End
