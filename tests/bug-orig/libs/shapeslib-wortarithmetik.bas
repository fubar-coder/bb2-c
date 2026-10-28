; shapeslib-wortarithmetik (#263) - LoadShape keeps the sizes of a shape
; as words: +26 (whole shape) is the low word of bytes per plane x depth.
; Here: an ILBM of 640x200 with 5 planes (16000 bytes per plane, 80000
; in all; uncompressed, built in memory and written to RAM:) is loaded;
; +26 is 80000 mod 65536 = 14464, and SaveShapes writes only that many
; bytes of the data behind the 32 byte block (file 14496 bytes).
; Correct: the whole shape is saved: 32 + 80000 = 80032 bytes.
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
n=80000
m=AllocMem(n+256,65536) : p=0
pl{$464F524D} : pl{0} : pl{$494C424D}
pl{$424D4844} : pl{20}
pw{640} : pw{200} : pw{0} : pw{0} : pb{5} : pb{0} : pb{0} : pb{0}
pw{0} : pb{10} : pb{11} : pw{640} : pw{200}
pl{$424F4459} : pl{n}
For i=0 To 79 : Poke.b m+p+i,i+1 : Next
p=p+n
Poke.l m+4,p-8
If WriteFile(0,"RAM:bo263.iff")
  WriteMem 0,m,p
  CloseFile 0
EndIf
FreeMem m,n+256
LoadShape 0,"RAM:bo263.iff"
KillFile "RAM:bo263.iff"
a=Addr Shape(0)
NPrint "shape ",Peek.w(a)," ",Peek.w(a+2)," ",Peek.w(a+4)," per plane ",Peek.w(a+22) AND $FFFF
SaveShapes 0,0,"RAM:bo263.shp"
If ReadFile(0,"RAM:bo263.shp")
  NPrint "file ",Lof(0)
  CloseFile 0
EndIf
KillFile "RAM:bo263.shp"
End
