; shapeslib-anim-xor (#266) - LoadShape of an ANIM reads the XOR bit
; (bits & 2 -> bmode) only from the first ANHD; the loop over the
; following frames finds the next ANHD and goes straight to the deltas.
; Here: an ANIM (op 5, interleave 1) built in memory and written to RAM:
; - frame 0 a 16x4 ILBM with 1 plane (BODY bytes 1..8), frame 1 with XOR
; (bits 2: the first byte becomes 1 XOR $F0 = $F1), frame 2 with set mode
; (bits 0, the first byte set to $33); LoadShape puts them in shapes 0-2.
; The original applies frame 2 with XOR as well: $F1 XOR $33 = $C2.
; Correct: frame 2 sets the byte: $33.
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
If WriteFile(0,"RAM:bo266.anim")
  WriteMem 0,m,p
  CloseFile 0
EndIf
LoadShape 0,"RAM:bo266.anim"
For s=0 To 2
  NPrint "frame ",s," ",Right$(Hex$(Peek.b(Peek.l(Addr Shape(s)+14)) AND 255),2)
Next
KillFile "RAM:bo266.anim"
FreeMem m,1024
End
