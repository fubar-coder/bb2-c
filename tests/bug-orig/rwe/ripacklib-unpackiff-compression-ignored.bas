; ripacklib-unpackiff-compression-ignored (#773) - UnpackIFF does not read
; the compression byte of the BMHD: every BODY is decoded as ByteRun1.
; The ILBM (built in memory) is 16x1, one plane, compression 0, its
; BODY the row $00,$C3; decoded as ByteRun1, $00 is a literal run of one
; byte: the row becomes $C3,$00.
; Correct: the uncompressed row is taken as it is, pixel 0 has colour 0
; and pixel 8 colour 1.
; ripacklib needs RIAmosFuncLib (LoadIFF calls its NextBank, BLoad, ...).
; @rwe ripacklib riamosfunclib
DEFTYPE .l
m=AllocMem_(6000,$10001)
If m=0 Then NPrint "no memory" : End
Poke.l m,$464F524D : Poke.l m+4,42 : Poke.l m+8,$494C424D
Poke.l m+12,$424D4844 : Poke.l m+16,20
Poke.w m+20,16 : Poke.w m+22,1 : Poke.b m+28,1 : Poke.b m+30,0
Poke.l m+40,$424F4459 : Poke.l m+44,2
Poke.b m+48,$00 : Poke.b m+49,$C3
BitMap 0,320,16,1
r=UnpackIFF(m,0)
NPrint Point(0,0)," ",Point(8,0)
FreeMem_ m,6000
End
