; shapeslib-anim-frames-maximum (#265) - LoadShape of an ANIM puts each
; further frame (ANHD/DLTA) into the NEXT shape object (LEA 32(a3),a2)
; without a check against the maximum of the shape objects: frames
; behind the last shape object are written over the memory behind the
; shape table (another library's objects or globals), and copyshape2
; frees what it finds there as a shape.
; Here: the 3-frame ANIM of shapeslib-anim-xor (op 5, interleave 1)
; built in memory, written to RAM: and loaded into shape 99, the last of
; the default maximum of 100.  The 64 bytes behind the shape table
; (Addr Shape(99)+32) are kept before the load and compared after it,
; then put back (so the end of the program does not meet the frames).
; Correct: the load stops at the last shape object, the 64 bytes behind
; it are unchanged ("behind unchanged"); shape 99 is frame 0.  The
; original writes frames 1 and 2 behind the table ("behind changed").
; @fixed #265
; @configs clib
DEFTYPE .l
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
Statement delta{bits.l,v.l}
  SHARED m,p
  ; FORM ILBM with ANHD (op 5, w,h,x,y, abstime, reltime, interleave 1,
  ; pad, bits, 16 pad bytes) and DLTA (16 plane offsets, plane 0 at 64)
  pl{$464F524D} : pl{4+48+8+68} : pl{$494C424D}
  pl{$414E4844} : pl{40}
  pb{5} : pb{0} : pw{16} : pw{4} : pw{0} : pw{0} : pl{0} : pl{1}
  pb{1} : pb{0} : pl{bits}
  For i.l=1 To 16 : pb{0} : Next
  pl{$444C5441} : pl{68}
  pl{64} : For i=1 To 15 : pl{0} : Next
  ; column 0: one op, 1 unique byte (row 0); column 1: no ops
  pb{1} : pb{$81} : pb{v}
  pb{0}
End Statement
m=AllocMem(1024,65536) : p=0
pl{$464F524D} : pl{0} : pl{$414E494D}
pl{$464F524D} : pl{4+28+16} : pl{$494C424D}
pl{$424D4844} : pl{20}
pw{16} : pw{4} : pw{0} : pw{0} : pb{1} : pb{0} : pb{0} : pb{0}
pw{0} : pb{10} : pb{11} : pw{320} : pw{200}
pl{$424F4459} : pl{8}
For i=1 To 8 : pb{i} : Next
delta{2,$F0}
delta{0,$33}
Poke.l m+4,p-8
If WriteFile(0,"RAM:bo265.anim")
  WriteMem 0,m,p
  CloseFile 0
EndIf
b=Addr Shape(99)+32
Dim k.l(15)
For i=0 To 15 : k(i)=Peek.l(b+i*4) : Next
LoadShape 99,"RAM:bo265.anim"
c=0
For i=0 To 15
  If Peek.l(b+i*4)<>k(i) Then c=c+1
  Poke.l b+i*4,k(i)
Next
If c=0 Then NPrint "behind unchanged" Else NPrint "behind changed"
NPrint "shape 99 ",ShapeWidth(99)," ",Right$(Hex$(Peek.b(Peek.l(Addr Shape(99)+14)) AND 255),2)
KillFile "RAM:bo265.anim"
FreeMem m,1024
End
