; rimapeditorlib-dbra-size0 (#895) - MapDataGrab, ShapeTo1616 and
; FillBitplane use a size - 1 as a DBRA counter without testing for 0:
; FillBitplane with a width below 16 writes width / 16 - 1 = $FFFF + 1
; words per row (128 KB).  Made safe: bitmap 5 is set up by hand (40
; bytes per row) with its plane 0 a block of the program's own of
; 256 KB; one row is filled, a marker 100000 bytes into the block shows
; the overrun.  The object is cleared again afterwards.
; Correct: a width below 16 fills no word, the marker stays.
; @rwe rimapeditorlib
DEFTYPE .l
b=AllocMem_(262144,$10001)
If b=0 Then NPrint "no memory" : End
Poke.l b+100000,$12345678
o=Addr BitMap(5)
Poke.w o,40 : Poke.w o+2,1 : Poke.w o+4,1 : Poke.l o+8,b
FillBitplane 5,8,1
v=Peek.l(b+100000)
Poke.w o,0 : Poke.w o+2,0 : Poke.w o+4,0 : Poke.l o+8,0
FreeMem_ b,262144
If v=$12345678 Then NPrint "marker kept" Else NPrint "marker overwritten"
End
