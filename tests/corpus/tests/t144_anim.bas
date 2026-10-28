; t144_anim - shapeslib LoadShape of an ANIM (op 5, interleave 1): the
; first frame (an uncompressed ILBM, 16x4, 1 plane) and two delta frames
; go into shapes 0, 1, 2.  The file is built here: frame 1 with a run of
; unique bytes and a run of one value, frame 2 with a row skip; the first
; ANHD asks for XOR mode (bits 2), which the original reads only once.
; Also LoadShape of a 2 plane ILBM with ByteRun1 runs (SaveBitmap).
DEFTYPE .l
Function.l cks{a.l,n.l}
  c.l=0
  If a=0 Then Function Return -1
  For i.l=0 To n-1
    c=(c*3+(Peek.b(a+i) AND 255)) AND $FFFFFF
  Next
  Function Return c
End Function
Statement shinfo{s.l}
  a.l=Addr Shape(s)
  Print "shape ",s," ",Peek.w(a)," ",Peek.w(a+2)," ",Peek.w(a+4)," ",Peek.w(a+6)," ",Hex$(Peek.w(a+8) AND $FFFF)
  NPrint " sz ",Peek.w(a+22)," ",Peek.w(a+26)," cookie ",Hex$(cks{Peek.l(a+18),Peek.w(a+22) AND $FFFF})
  d.l=Peek.l(a+14) : s$=""
  If d<>0
    For i.l=0 To (Peek.w(a+26) AND $FFFF)-1 : s$=s$+Right$(Hex$(Peek.b(d+i) AND 255),2) : Next
  EndIf
  NPrint "  [",s$,"]"
End Statement
Statement pb{v.l}
  SHARED m,p
  Poke.b m+p,v : p=p+1
End Statement
Statement pw{v.l}
  pb{(v LSR 8) AND 255} : pb{v AND 255}
End Statement
Statement pl{v.l}
  pw{(v LSR 16) AND $FFFF} : pw{v AND $FFFF}
End Statement
Statement delta{c0.l}
  SHARED m,p
  ; ANHD: op 5, mask, w,h,x,y, abstime, reltime, interleave 1, pad, bits
  pl{$464F524D} : pl{4+48+8+64+8} : pl{$494C424D}
  pl{$414E4844} : pl{40}
  pb{5} : pb{0} : pw{16} : pw{4} : pw{0} : pw{0} : pl{0} : pl{1}
  pb{1} : pb{0} : pl{2}
  For i.l=1 To 16 : pb{0} : Next
  ; DLTA: 16 offsets (plane 0 at 64), then the columns
  pl{$444C5441} : pl{64+8}
  pl{64} : For i=1 To 15 : pl{0} : Next
  If c0=0
    pb{1} : pb{$82} : pb{$AA} : pb{$BB}
    pb{1} : pb{0} : pb{3} : pb{$55}
  Else
    pb{2} : pb{1} : pb{$81} : pb{$FF}
    pb{0}
  EndIf
End Statement
m=AllocMem(1024,65536) : p=0
pl{$464F524D} : pl{0} : pl{$414E494D}
pl{$464F524D} : pl{4+28+16} : pl{$494C424D}
pl{$424D4844} : pl{20}
pw{16} : pw{4} : pw{0} : pw{0} : pb{1} : pb{0} : pb{0} : pb{0}
pw{0} : pb{10} : pb{11} : pw{320} : pw{200}
pl{$424F4459} : pl{8}
For i=1 To 8 : pb{i} : Next
delta{0}
delta{1}
Poke.l m+4,p-8
NPrint "anim bytes ",p
If WriteFile(0,"RAM:t144.anim")
  WriteMem 0,m,p
  CloseFile 0
EndIf
LoadShape 0,"RAM:t144.anim"
shinfo{0} : shinfo{1} : shinfo{2}
NPrint "frame 3 ",Peek.w(Addr Shape(3))
AutoCookie Off
LoadShape 5,"RAM:t144.anim"
shinfo{5} : shinfo{6} : shinfo{7}
AutoCookie On
; a 2 plane ILBM with runs (ByteRun1)
BitMap 0,48,6,2
b=Addr BitMap(0)
For i=0 To 35
  v=$33 : If i>10 Then v=i*7
  If i>25 Then v=$C0
  Poke.b Peek.l(b+8)+i,v : Poke.b Peek.l(b+12)+i,255-v
Next
SaveBitmap 0,"RAM:t144.iff"
LoadShape 10,"RAM:t144.iff"
shinfo{10}
KillFile "RAM:t144.anim" : KillFile "RAM:t144.iff"
FreeMem m,1024
End
