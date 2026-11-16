; shapeslib-loadshape-height-zero (#1054) - LoadShape takes the height of
; the BMHD without a check: a height of 0 makes a data block of 0 bytes,
; and the row loop (the counter decremented before it is tested) then
; runs 65536 rows and writes them behind that block.
; Here: an ILBM of 16 x 0 pixels with 1 plane (a BODY of 4 bytes), built
; in memory and written to RAM:; SetErr catches a runtime error.  The
; program runs through inject with guard (runwrap=): a pattern behind
; every AllocMem, checked at FreeMem and at the end.
; Correct: LoadShape refuses the file ("Illegal Shape Size", caught:
; "refused"), the shape stays empty, nothing is written.  The original
; writes the BODY behind the block of 0 bytes until the data of the file
; ends ("Can't Read File" of ilbmifflib).
; @fixed #1054
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
pw{16} : pw{0} : pw{0} : pw{0} : pb{1} : pb{0} : pb{0} : pb{0}
pw{0} : pb{10} : pb{11} : pw{320} : pw{200}
pl{$424F4459} : pl{n}
p=p+n
Poke.l m+4,p-8
If WriteFile(0,"RAM:bo1054h.iff")
  WriteMem 0,m,p
  CloseFile 0
EndIf
FreeMem m,n+256
LoadShape 0,"RAM:bo1054h.iff"
KillFile "RAM:bo1054h.iff"
NPrint "loaded, width ",Peek.w(Addr Shape(0))
End
