; animlib-zero-dimensions (#402) - InitAnim (and NextFrame) count rows,
; planes and columns with SUBQ #1 ... DBF: a BMHD with height, depth or
; width 0 gives 65536 iterations, far beyond the bitmap.
; Here: an ANIM (generated) of height 0 whose BODY holds 65536 rows of 2
; bytes (row 1 repeats $11, the others $FF).  To keep the writes inside
; the bitmap the program sets the bytes per row of BitMap 0 to 0 during
; InitAnim (every row lands on row 0 of the plane) and restores it.
; Correct: a picture of height 0 decodes no row, the plane stays 0.  The
; original decodes all 65536 rows (the last one leaves $FFFF).
; @configs clib
; @pass file=height0.anim|@TESTDATA@/animlib-zero-dimensions.anim
DEFTYPE .l
BitMap 0,16,8,1
b=Addr BitMap(0)
LoadAnim 0,"CORPUS:height0.anim"
a=Addr Anim(0)
NPrint "height ",Peek.w(a+2)," body ",Peek.l(a+14)
w=Peek.w(b)
Poke.w b,0
InitAnim 0
Poke.w b,w
NPrint "row ",Hex$(Peek.w(Peek.l(b+8)) AND $FFFF)
End
