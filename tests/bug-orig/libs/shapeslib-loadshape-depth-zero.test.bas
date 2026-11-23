; shapeslib-loadshape-depth-zero (#1052) - LoadShape takes the number of
; planes from the BMHD without a check: 0 planes make a shape of depth 0
; whose data block has 0 bytes, and the plane loop (DBF on depth - 1)
; then writes 65536 planes of every row behind that block.
; Here: an ILBM of 16 x 2 pixels with 0 planes (a BODY of 4 bytes),
; built in memory and written to CORPUS:; SetErr catches the runtime error.
; The program runs through inject with guard (runwrap=): a pattern
; behind every AllocMem, checked at FreeMem and at the end.
; Correct: LoadShape refuses the file ("Illegal Shape Depth", caught: the
; handler checks that the shape stays empty and ends the program),
; nothing is written behind an allocation.  The original writes the BODY
; behind the block of 0 bytes until the data of the file ends ("Can't
; Read File" of ilbmifflib): the shape has width 16, inject reports the
; write behind the allocation of 8 bytes (the deviations of libs=cd).
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

!TestCase{depth_zero}
  ; setup: the ILBM in memory, written to RAM:
  n=4
  m=AllocMem(n+256,65537) : p=0
  pl{$464F524D} : pl{0} : pl{$494C424D}
  pl{$424D4844} : pl{20}
  pw{16} : pw{2} : pw{0} : pw{0} : pb{0} : pb{0} : pb{0} : pb{0}
  pw{0} : pb{10} : pb{11} : pw{16} : pw{2}
  pl{$424F4459} : pl{n}
  p=p+n
  Poke.l m+4,p-8
  ok=WriteFile(0,"CORPUS:bo1052.iff")
  !Require{ok,"file_written"}
  WriteMem 0,m,p
  CloseFile 0
  FreeMem m,n+256
  SetErr
    KillFile "CORPUS:bo1052.iff"
    !CheckEq{Peek.w(Addr Shape(0)),0,"shape_width"}
    End
  End SetErr
  ; act
  LoadShape 0,"CORPUS:bo1052.iff"
  !Check{0,"not_refused"}
!EndTestCase
