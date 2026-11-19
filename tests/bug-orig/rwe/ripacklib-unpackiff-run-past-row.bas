; ripacklib-unpackiff-run-past-row (#776) - UnpackIFF writes every run
; completely, also past the end of the row.  The ILBM (built in memory)
; is 16x2, one plane, ByteRun1; its BODY is $F6,$FF (11 bytes of $FF for
; a row of 2 bytes), then $01,$00,$00 (row 1: 2 bytes of $00).  The
; original writes the 11 bytes into rows 0 to 5 of the bitmap (2 bytes a
; row); row 1 is then overwritten by the next run.
; Correct: the run is cut at the end of row 0: row 0 has colour 1, rows 1
; to 5 colour 0.
; ripacklib needs RIAmosFuncLib (LoadIFF calls its NextBank, BLoad, ...).
; @fixed #776
; @rwe ripacklib riamosfunclib
DEFTYPE .l
m=AllocMem_(6000,$10001)
If m=0 Then NPrint "no memory" : End
Poke.l m,$464F524D : Poke.l m+4,46 : Poke.l m+8,$494C424D
Poke.l m+12,$424D4844 : Poke.l m+16,20
Poke.w m+20,16 : Poke.w m+22,2 : Poke.b m+28,1 : Poke.b m+30,1
Poke.l m+40,$424F4459 : Poke.l m+44,5
Poke.b m+48,$F6 : Poke.b m+49,$FF : Poke.b m+50,$01
BitMap 0,16,8,1
r=UnpackIFF(m,0)
NPrint r," ",Point(0,0)," ",Point(0,1)," ",Point(0,2)," ",Point(0,3)," ",Point(0,5)
FreeMem_ m,6000
End
