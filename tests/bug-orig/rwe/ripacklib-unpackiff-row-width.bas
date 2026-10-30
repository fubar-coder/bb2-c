; ripacklib-unpackiff-row-width (#775) - UnpackIFF takes the bytes per row
; as width/8 instead of the word-aligned ILBM row.  The ILBM (built in
; memory) is 8x2, one plane, ByteRun1; each row has 2 bytes in the file,
; each byte a literal run of its own: row 0 $FF,$00, row 1 $0F,$00.  The
; original decodes 1 byte per row: row 1 gets row 0's pad byte ($00).
; Correct: row 0 is $FF, row 1 $0F (pixel 4 of row 1 has colour 1).
; ripacklib needs RIAmosFuncLib (LoadIFF calls its NextBank, BLoad, ...).
; @rwe ripacklib riamosfunclib
DEFTYPE .l
m=AllocMem_(6000,$10001)
If m=0 Then NPrint "no memory" : End
Poke.l m,$464F524D : Poke.l m+4,48 : Poke.l m+8,$494C424D
Poke.l m+12,$424D4844 : Poke.l m+16,20
Poke.w m+20,8 : Poke.w m+22,2 : Poke.b m+28,1 : Poke.b m+30,1
Poke.l m+40,$424F4459 : Poke.l m+44,8
Poke.l m+48,$00FF0000 : Poke.l m+52,$000F0000
BitMap 0,320,16,1
r=UnpackIFF(m,0)
NPrint Point(0,0)," ",Point(4,1)," ",Point(0,1)
FreeMem_ m,6000
End
