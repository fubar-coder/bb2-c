; shapeslib-loadshape-xplane-boundary (#1055) - the largest plane LoadShape
; still loads: since #1055 it refuses a picture whose plane with the
; extra word of every row, (bytes per row + 2) x height, is 64K or more.
; Here: an ILBM of 992 x 520 with 1 plane (64480 bytes per plane, 65520
; with the extra word), uncompressed, built in memory and written to
; RAM:; SetErr catches a runtime error.
; Correct: LoadShape loads it, +22 = 64480 and +24 = 65520.
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
n=64480
m=AllocMem(n+256,65537) : p=0
pl{$464F524D} : pl{0} : pl{$494C424D}
pl{$424D4844} : pl{20}
pw{992} : pw{520} : pw{0} : pw{0} : pb{1} : pb{0} : pb{0} : pb{0}
pw{0} : pb{10} : pb{11} : pw{992} : pw{520}
pl{$424F4459} : pl{n}
p=p+n
Poke.l m+4,p-8
If WriteFile(0,"RAM:rg1055.iff")
  WriteMem 0,m,p
  CloseFile 0
EndIf
FreeMem m,n+256
LoadShape 0,"RAM:rg1055.iff"
KillFile "RAM:rg1055.iff"
s=Addr Shape(0)
NPrint "loaded, per plane ",Peek.w(s+22) AND $FFFF," with the extra word ",Peek.w(s+24) AND $FFFF
End
