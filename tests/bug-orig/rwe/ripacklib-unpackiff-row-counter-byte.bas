; ripacklib-unpackiff-row-counter-byte (#774) - UnpackIFF counts the bytes
; of a row in a signed byte: rows of 129 bytes and more are "done" before
; anything is read.  The ILBM (built in memory) is 1040x2, one plane,
; ByteRun1, 130 bytes a row: row 0 is 130 bytes of $FF, row 1 128 bytes
; of $00 and 2 bytes of $0F.  The original unpacks nothing.
; Correct: pixel 0 and 1039 of row 0 have colour 1; of row 1 pixel 0 has
; colour 0, pixel 1039 colour 1.
; ripacklib needs RIAmosFuncLib (LoadIFF calls its NextBank, BLoad, ...).
; @fixed #774
; @rwe ripacklib riamosfunclib
DEFTYPE .l
m=AllocMem_(6000,$10001)
If m=0 Then NPrint "no memory" : End
Poke.l m,$464F524D : Poke.l m+4,48 : Poke.l m+8,$494C424D
Poke.l m+12,$424D4844 : Poke.l m+16,20
Poke.w m+20,1040 : Poke.w m+22,2 : Poke.b m+28,1 : Poke.b m+30,1
Poke.l m+40,$424F4459 : Poke.l m+44,8
Poke.l m+48,$81FFFFFF : Poke.l m+52,$8100FF0F
BitMap 0,1040,2,1
r=UnpackIFF(m,0)
NPrint r," ",Point(0,0)," ",Point(1039,0)," ",Point(0,1)," ",Point(1039,1)
FreeMem_ m,6000
End
