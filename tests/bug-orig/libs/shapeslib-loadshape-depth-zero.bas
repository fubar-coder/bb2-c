; shapeslib-loadshape-depth-zero (#1052) - LoadShape takes the number of
; planes from the BMHD without a check: 0 planes make a shape of depth 0
; whose data block has 0 bytes, and the plane loop (DBF on depth - 1)
; then writes 65536 planes of every row behind that block.
; Here: an ILBM of 16 x 2 pixels with 0 planes (a BODY of 4 bytes),
; built in memory and written to RAM:; SetErr catches a runtime error.
; The program runs through inject with guard (runwrap=): a pattern
; behind every AllocMem, checked at FreeMem and at the end.
; Correct: LoadShape refuses the file ("Illegal Shape Depth", caught:
; "refused"), the shape stays empty, nothing is written.  The original
; writes the BODY behind the block of 0 bytes until the data of the file
; ends ("Can't Read File" of ilbmifflib).
; @fixed #1052
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
SetErr
  NPrint "refused, shape width ",Peek.w(Addr Shape(0))
  End
End SetErr
n=4
m=AllocMem(n+256,65537) : p=0
pl{$464F524D} : pl{0} : pl{$494C424D}
pl{$424D4844} : pl{20}
pw{16} : pw{2} : pw{0} : pw{0} : pb{0} : pb{0} : pb{0} : pb{0}
pw{0} : pb{10} : pb{11} : pw{16} : pw{2}
pl{$424F4459} : pl{n}
p=p+n
Poke.l m+4,p-8
If WriteFile(0,"RAM:bo1052.iff")
  WriteMem 0,m,p
  CloseFile 0
EndIf
FreeMem m,n+256
LoadShape 0,"RAM:bo1052.iff"
KillFile "RAM:bo1052.iff"
NPrint "loaded, depth ",Peek.w(Addr Shape(0)+4)
End
