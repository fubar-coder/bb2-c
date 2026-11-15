; shapeslib-loadshape-blitsize-height (#1046) - a shape keeps its blit
; size in one word (height << 6 | words + 1, the blitter's BLTSIZE): the
; height field has 10 bits, so a shape taller than 1023 rows (1024 works
; as the field 0) gets a wrong blit size, and its blits draw a wrong
; height.  LoadShape takes the height from the file without a check.
; Here: an ILBM of 16 x 1100 pixels, 1 plane, uncompressed, built in
; memory and written to RAM:; SetErr catches a runtime error.
; Correct: the shape format cannot hold the height: LoadShape refuses the
; file ("Can't Read/Write Shapes File", caught: "refused"), the shape
; stays empty.  The original loads it with the blit size word
; (1100 << 6 | 2) AND $FFFF = $1302 (a height of 76 rows).
; @fixed #1046
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
SetErr
  NPrint "refused, shape width ",Peek.w(Addr Shape(0))
  End
End SetErr
n=2200
m=AllocMem(n+256,65537) : p=0
pl{$464F524D} : pl{0} : pl{$494C424D}
pl{$424D4844} : pl{20}
pw{16} : pw{1100} : pw{0} : pw{0} : pb{1} : pb{0} : pb{0} : pb{0}
pw{0} : pb{10} : pb{11} : pw{16} : pw{1100}
pl{$424F4459} : pl{n}
p=p+n
Poke.l m+4,p-8
If WriteFile(0,"RAM:bo1046h.iff")
  WriteMem 0,m,p
  CloseFile 0
EndIf
FreeMem m,n+256
LoadShape 0,"RAM:bo1046h.iff"
KillFile "RAM:bo1046h.iff"
NPrint "loaded, height ",Peek.w(Addr Shape(0)+2)," blit size ",Hex$(Peek.w(Addr Shape(0)+8) AND $FFFF)
End
