; shapeslib-loadshape-blitsize-height (#1046) - a shape keeps its blit
; size in one word (height << 6 | words + 1, the blitter's BLTSIZE): the
; height field has 10 bits, so a shape taller than 1023 rows (1024 works
; as the field 0) gets a wrong blit size, and its blits draw a wrong
; height.  LoadShape takes the height from the file without a check.
; Here: an ILBM of 16 x 1100 pixels, 1 plane, uncompressed, built in
; memory and written to CORPUS:; SetErr catches a runtime error.
; Correct: the shape format cannot hold the height: LoadShape refuses the
; file ("Can't Read/Write Shapes File", caught: "refused"), the shape
; stays empty.  The original loads it with the blit size word
; (1100 << 6 | 2) AND $FFFF = $1302 (a height of 76 rows).
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

!TestCase{blitsize_height}
  n=2200
  m=AllocMem(n+256,65537) : p=0
  pl{$464F524D} : pl{0} : pl{$494C424D}
  pl{$424D4844} : pl{20}
  pw{16} : pw{1100} : pw{0} : pw{0} : pb{1} : pb{0} : pb{0} : pb{0}
  pw{0} : pb{10} : pb{11} : pw{16} : pw{1100}
  pl{$424F4459} : pl{n}
  p=p+n
  Poke.l m+4,p-8
  ok=WriteFile(0,"CORPUS:bo1046h.iff")
  !Require{ok,"file_written"}
  WriteMem 0,m,p
  CloseFile 0
  FreeMem m,n+256
  SetErr
    KillFile "CORPUS:bo1046h.iff"
    !CheckEq{Peek.w(Addr Shape(0)),0,"shape_width"}
    End
  End SetErr
  LoadShape 0,"CORPUS:bo1046h.iff"
  !Check{0,"not_refused"}
!EndTestCase
