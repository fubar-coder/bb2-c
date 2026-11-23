; shapeslib-loadshape-xplane-64k (#1055) - a shape keeps its plane sizes
; in words: +22 bytes per plane, +24 the same with the extra word of
; every row ((bytes per row + 2) x height: what BBlit saves of the bitmap
; per plane and adds to its buffer pointer).  LoadShape stores the low
; word of (bytes per row + 2) x height in +24 without a check: a picture
; whose plane is just under 64K but whose plane with the extra word is
; not gets a wrong +24.
; Here: an ILBM of 992 x 528 with 1 plane (65472 bytes per plane, 66528
; with the extra word: the low word 992), uncompressed, built in memory
; and written to CORPUS:; SetErr catches a runtime error.
; Correct: the shape format cannot hold the plane with the extra word:
; LoadShape refuses the file ("Can't Read/Write Shapes File", caught:
; "refused"), as InitShape and GetaShape refuse such a shape with
; runtime error checking; the shape stays empty.  The original loads it
; with +24 = 992.
DEFTYPE .l
Statement pb{v.l}
  SHARED m,p
  Poke.b m+p,v : p=p+1
End Statement
Statement pw{v.l}
  pb{(v LSR 8) AND 255} : pb{v AND 255}
End Statement
Statement pl{v.l}
  pw{(v LSR 16) AND $FFFF} : pw{v AND $FFFF}
End Statement

!TestCase{xplane_64k}
  n=65472
  m=AllocMem(n+256,65537) : p=0
  pl{$464F524D} : pl{0} : pl{$494C424D}
  pl{$424D4844} : pl{20}
  pw{992} : pw{528} : pw{0} : pw{0} : pb{1} : pb{0} : pb{0} : pb{0}
  pw{0} : pb{10} : pb{11} : pw{992} : pw{528}
  pl{$424F4459} : pl{n}
  p=p+n
  Poke.l m+4,p-8
  ok=WriteFile(0,"CORPUS:bo1055.iff")
  !Require{ok,"file_written"}
  WriteMem 0,m,p
  CloseFile 0
  FreeMem m,n+256
  SetErr
    KillFile "CORPUS:bo1055.iff"
    !CheckEq{Peek.w(Addr Shape(0)),0,"shape_width"}
    End
  End SetErr
  LoadShape 0,"CORPUS:bo1055.iff"
  !Check{0,"not_refused"}
!EndTestCase
