; shapeslib-loadshape-plane-step (#263) - LoadShape decodes the BODY row
; by row and plane by plane and goes from the end of a row of one plane to
; the same row of the next plane with ADD d7,a2, d7 = bytes per plane -
; bytes per row as a word (sign extended): for planes of 32K and more
; the step goes backwards and the planes from the second one on are
; written before the data block.
; Here: an ILBM of 640x420 with 2 planes (33600 bytes per plane, the step
; 33520), uncompressed, built in memory and written to RAM:; the first
; byte of row 0 of plane 1 is $5A, of plane 0 $11.  The program runs
; through inject with guard (runwrap=): a pattern behind every AllocMem,
; checked at FreeMem and at the end.  After LoadShape the first bytes of
; the two planes of the shape (+14 data, +22 bytes per plane).
; Correct: plane 0 starts with $11, plane 1 with $5A, nothing is written
; outside an allocation.  The original writes plane 1 before the data
; block.
; @fixed #263
; @configs clib
; @pass runwrap=@INJECT@|guard
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
n=67200
m=AllocMem(n+256,65537) : p=0
pl{$464F524D} : pl{0} : pl{$494C424D}
pl{$424D4844} : pl{20}
pw{640} : pw{420} : pw{0} : pw{0} : pb{2} : pb{0} : pb{0} : pb{0}
pw{0} : pb{10} : pb{11} : pw{640} : pw{420}
pl{$424F4459} : pl{n}
Poke.b m+p,$11 : Poke.b m+p+80,$5A
p=p+n
Poke.l m+4,p-8
If WriteFile(0,"RAM:bo263s.iff")
  WriteMem 0,m,p
  CloseFile 0
EndIf
FreeMem m,n+256
LoadShape 0,"RAM:bo263s.iff"
KillFile "RAM:bo263s.iff"
a=Addr Shape(0)
d=Peek.l(a+14)
NPrint "plane 0 ",Hex$(Peek.b(d) AND 255)," plane 1 ",Hex$(Peek.b(d+(Peek.w(a+22) AND $FFFF)) AND 255)
Free Shape 0
End
