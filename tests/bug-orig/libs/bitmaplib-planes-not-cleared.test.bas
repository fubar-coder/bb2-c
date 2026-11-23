; bitmaplib-planes-not-cleared (#80) - BitMap (extbitmap) sets only the
; plane pointers up to the depth, _freebitmap does not clear them: a BitMap
; object that had 5 planes and gets 2 keeps the pointers of planes 2-4
; into the freed memory.  DecodeILBM writes all planes of the ILBM through
; these pointers (into freed memory); here the program reads the pointers
; of the object (+8: Planes[8]) back.
; Correct: the pointers above the depth are cleared (0).
!TestCase{planes_cleared}
  BitMap 0,16,1,5
  o.l=Addr BitMap(0)
  !Check{Peek.l(o+16)<>0,"plane2_set"}
  !Check{Peek.l(o+20)<>0,"plane3_set"}
  !Check{Peek.l(o+24)<>0,"plane4_set"}
  BitMap 0,16,1,2
  !CheckEq{Peek.b(o+5),2,"depth"}
  !CheckEq{Peek.l(o+16),0,"plane2_cleared"}
  !CheckEq{Peek.l(o+20),0,"plane3_cleared"}
  !CheckEq{Peek.l(o+24),0,"plane4_cleared"}
!EndTestCase
