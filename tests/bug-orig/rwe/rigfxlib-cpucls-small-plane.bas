; rigfxlib-cpucls-small-plane (#682) - CPUCls clears each plane from its
; end downwards in blocks of 40 bytes, DBRA on blocks - 1: a plane of
; less than 40 bytes (here 2 bytes x 16 rows = 32) gives the counter
; $FFFF, 65536 blocks (2621440 bytes) below the end of the plane are
; cleared.  Made safe: bitmap 5 is set up by hand with its plane at the
; end of a block of the program's own (3 MB); a marker 2 MB below the
; end of the plane shows the clearing.  The object is cleared again
; afterwards.
; Correct: CPUCls clears the 32 bytes of the plane, the marker stays.
; rigfxlib needs RIAmosFuncLib (SaveCMAP calls its BSave).
; @rwe rigfxlib riamosfunclib
DEFTYPE .l
b=AllocMem_(3145728,$10001)
If b=0 Then NPrint "no memory" : End
Poke.l b+1048576,$12345678
o=Addr BitMap(5)
Poke.w o,2 : Poke.w o+2,16 : Poke.w o+4,1 : Poke.l o+8,b+3145728-32
CPUCls 5
v=Peek.l(b+1048576)
Poke.w o,0 : Poke.w o+2,0 : Poke.w o+4,0 : Poke.l o+8,0
FreeMem_ b,3145728
If v=$12345678 Then NPrint "marker kept" Else NPrint "marker cleared"
End
