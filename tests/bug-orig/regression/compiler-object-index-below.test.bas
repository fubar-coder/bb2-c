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
; A regression test of the fix of #939: the original already gives this
; (correct) result.
; (The maximum of 512 BitMap objects is the job's line "max=160|512".)

DEFTYPE .l

!TestCase{object_index_below}
  o=Addr BitMap(0)+32704
  !CheckEq{Addr BitMap(511)-Addr BitMap(0),32704,"object_distance"}
  !CheckEq{4+Addr BitMap(511)-o,4,"object_address_long"}
  BitMap 511,16,8,1
  !CheckEq{Peek.w(o),2,"bytes_per_row"}
  !CheckEq{Peek.w(o+62),-1,"own_memory_set"}
  BitMap 0,16,8,1
  Use BitMap 511
  Plot 0,0,1
  !CheckEq{Peek.b(Peek.l(o+8)) AND 255,128,"plot_into_plane"}
  Free BitMap 511
  !CheckEq{Peek.w(o+62),0,"own_memory_cleared"}
!EndTestCase
