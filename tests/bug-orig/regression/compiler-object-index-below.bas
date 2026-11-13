; compiler-object-index-below (#939) - the forms of
; compiler-object-index-long with a maximum whose last object still lies
; within $7FFF: the BitMap maximum is 512 (bitmaplib = 160, objects of 64
; bytes), BitMap 511 is 32704 bytes behind BitMap 0; the object offset
; stays a word (lsl.w, add.w), as in the original.  o is the object's
; address computed as a long.
; Correct: Addr BitMap(511) - Addr BitMap(0) = 32704; BitMap 511 sets the
; bytes per row of that object (2); Use BitMap 511 makes Plot draw into
; its plane ($80 in the first byte); Free BitMap 511 clears its "own
; memory" field (+62).
; A regression test of the fix of #939: the original already gives this (correct) result.
; @pass jobline=max=160|512
DEFTYPE .l
o=Addr BitMap(0)+32704
NPrint Addr BitMap(511)-Addr BitMap(0)
NPrint 4+Addr BitMap(511)-o
BitMap 511,16,8,1
NPrint Peek.w(o)," ",Peek.w(o+62)
BitMap 0,16,8,1
Use BitMap 511
Plot 0,0,1
NPrint Peek.b(Peek.l(o+8)) AND 255
Free BitMap 511
NPrint Peek.w(o+62)
End
