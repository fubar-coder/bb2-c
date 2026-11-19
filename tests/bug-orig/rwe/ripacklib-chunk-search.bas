; ripacklib-chunk-search (#766) - UnpackIFF and ILBMPalette do not walk
; the IFF chunks: they compare a long at every even address, BMHD and
; CMAP within about 5 KB of the given address, BODY within 5 KB of the
; BMHD.  The ILBM (built in memory) is 16x1, one plane, ByteRun1: BMHD,
; an ANNO chunk of 6000 bytes, CMAP (one colour $F0 $80 $10), BODY (the
; row $C3,$3C).  At the start of the ANNO's text stands "BODY" with the
; row $00,$00 behind it.  The original unpacks the "BODY" in the ANNO
; (all pixels 0) and finds no CMAP; with the text gone it finds no BODY.
; Correct: the chunks are found by walking the FORM: both times the row
; $C3,$3C (pixels 0 and 10 colour 1, 2 and 8 colour 0), and the CMAP.
; ripacklib needs RIAmosFuncLib (LoadIFF calls its NextBank, BLoad, ...).
; @fixed #766
; @rwe ripacklib riamosfunclib
DEFTYPE .l
m=AllocMem_(7000,$10001)
If m=0 Then NPrint "no memory" : End
Poke.l m,$464F524D : Poke.l m+4,6064 : Poke.l m+8,$494C424D
Poke.l m+12,$424D4844 : Poke.l m+16,20
Poke.w m+20,16 : Poke.w m+22,1 : Poke.b m+28,1 : Poke.b m+30,1
Poke.l m+40,$414E4E4F : Poke.l m+44,6000
Poke.l m+48,$424F4459 : Poke.l m+52,3 : Poke.b m+56,1
Poke.l m+6048,$434D4150 : Poke.l m+6052,3
Poke.b m+6056,$F0 : Poke.b m+6057,$80 : Poke.b m+6058,$10
Poke.l m+6060,$424F4459 : Poke.l m+6064,3
Poke.b m+6068,$01 : Poke.b m+6069,$C3 : Poke.b m+6070,$3C
BitMap 0,320,16,1
r=UnpackIFF(m,0)
NPrint "unpack ",r," ",Point(0,0)," ",Point(2,0)," ",Point(8,0)," ",Point(10,0)
Cls
Poke.l m+48,0
r=UnpackIFF(m,0)
NPrint "unpack ",r," ",Point(0,0)," ",Point(2,0)," ",Point(8,0)," ",Point(10,0)
r=ILBMPalette(m,0)
o=Addr Palette(0) : n=Peek.w(o+4)
a$="" : If n>0 Then e=Peek.l(o)+4 : a$=Hex$(Peek.b(e) AND 255)
NPrint "palette ",r," ",n," ",a$
FreeMem_ m,7000
End
