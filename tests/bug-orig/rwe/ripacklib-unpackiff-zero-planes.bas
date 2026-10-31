; ripacklib-unpackiff-zero-planes (#777) - UnpackIFF checks nPlanes only
; against the bitmap's depth; 0 passes, SUBQ.b #1 makes the depth 255 and
; 256 plane pointers are copied into the table of 8, over the variables
; and the code behind it (the C build: over other data).  The ILBM
; (built in memory) is 16x1 with nPlanes 0; ILBMPalette afterwards runs
; the code behind the table.
; Correct: an ILBM without planes is refused, the program goes on.
; ripacklib needs RIAmosFuncLib (LoadIFF calls its NextBank, BLoad, ...).
; @rwe ripacklib riamosfunclib
; @pass timeout=30
DEFTYPE .l
m=AllocMem_(6000,$10001)
If m=0 Then NPrint "no memory" : End
Poke.l m,$464F524D : Poke.l m+4,44 : Poke.l m+8,$494C424D
Poke.l m+12,$424D4844 : Poke.l m+16,20
Poke.w m+20,16 : Poke.w m+22,1 : Poke.b m+28,0 : Poke.b m+30,1
Poke.l m+40,$424F4459 : Poke.l m+44,4
Poke.l m+48,$00FF00FF
BitMap 0,320,16,1
NPrint "before"
r=UnpackIFF(m,0)
r=ILBMPalette(m,0)
NPrint "after"
FreeMem_ m,6000
End
