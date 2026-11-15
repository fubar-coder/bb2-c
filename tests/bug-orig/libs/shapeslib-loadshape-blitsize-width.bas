; shapeslib-loadshape-blitsize-width (#1046) - a shape keeps its blit
; size in one word (height << 6 | words + 1, the blitter's BLTSIZE): the
; width field has 6 bits: from 63 words (993 pixels) on the "+ 1" carries
; into the height field (from 64 words on the words OR into it), and the
; blits draw a wrong size.  LoadShape takes the width without a check.
; Here: an ILBM of 1008 x 2 pixels (63 words), 1 plane, uncompressed,
; built in memory and written to RAM:; SetErr catches a runtime error.
; Correct: the shape format cannot hold the width: LoadShape refuses the
; file ("Can't Read/Write Shapes File", caught: "refused"), the shape
; stays empty.  The original loads it with the blit size word
; (2 << 6 | 63) + 1 = $00C0 (3 rows of 64 words).
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
n=252
m=AllocMem(n+256,65537) : p=0
pl{$464F524D} : pl{0} : pl{$494C424D}
pl{$424D4844} : pl{20}
pw{1008} : pw{2} : pw{0} : pw{0} : pb{1} : pb{0} : pb{0} : pb{0}
pw{0} : pb{10} : pb{11} : pw{1008} : pw{2}
pl{$424F4459} : pl{n}
p=p+n
Poke.l m+4,p-8
If WriteFile(0,"RAM:bo1046w.iff")
  WriteMem 0,m,p
  CloseFile 0
EndIf
FreeMem m,n+256
LoadShape 0,"RAM:bo1046w.iff"
KillFile "RAM:bo1046w.iff"
NPrint "loaded, width ",Peek.w(Addr Shape(0))," blit size ",Hex$(Peek.w(Addr Shape(0)+8) AND $FFFF)
End
