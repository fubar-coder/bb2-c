; iffmakelib-saveshape-tempbm (#326) - SaveShape describes the shape as a
; BitMap in the static buffer tempbm (room for 14 plane pointers) and
; enters one pointer per plane without looking at the depth: from depth 15
; on the pointers run into the variables behind it, the 15th into usewid,
; the width SaveShape writes into the BMHD.
; Such a shape comes from a file: LoadShape takes the depth of the BMHD
; as it is. Here an ILBM of 16x1 pixels with 15 planes (uncompressed),
; built in memory and written to RAM:, loaded with LoadShape and saved
; with SaveShape; the program reads the BMHD of the saved file back (the
; width in it is the high word of the 15th pointer, an address: not
; printed).
; Runtime error checking off: SetErr catches a runtime error.
; Correct: a shape deeper than tempbm holds is not written over the
; buffer: SaveShape stops with a runtime error.
; @fixed #326
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
  NPrint "runtime error"
  KillFile "RAM:bo326.iff" : KillFile "RAM:bo326o.iff"
  End
End SetErr
m=AllocMem(256,65536) : p=0
pl{$464F524D} : pl{0} : pl{$494C424D}
pl{$424D4844} : pl{20}
pw{16} : pw{1} : pw{0} : pw{0} : pb{15} : pb{0} : pb{0} : pb{0}
pw{0} : pb{10} : pb{11} : pw{16} : pw{1}
pl{$424F4459} : pl{30}
For i=1 To 15 : pw{i*$101} : Next
Poke.l m+4,p-8
If WriteFile(0,"RAM:bo326.iff")
  WriteMem 0,m,p
  CloseFile 0
EndIf
LoadShape 0,"RAM:bo326.iff"
NPrint "shape ",ShapeWidth(0)," ",Peek.w(Addr Shape(0)+4)
SaveShape 0,"RAM:bo326o.iff"
If ReadFile(0,"RAM:bo326o.iff")
  ReadMem 0,m,40
  CloseFile 0
EndIf
NPrint "saved, planes ",Peek.b(m+28)
KillFile "RAM:bo326.iff" : KillFile "RAM:bo326o.iff"
FreeMem m,256
End
