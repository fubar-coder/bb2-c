; shapeslib-loadshape-width-zero (#1054) - LoadShape takes the width of
; the BMHD without a check: a width of 0 makes 0 bytes per row and a data
; block of 0 bytes, and the byte loop of each row (DBF on bytes per row
; - 1) then reads 65536 bytes per row and plane and writes them behind
; that block.
; Here: an ILBM of 0 x 2 pixels with 1 plane (a BODY of 4 bytes), built
; in memory and written to CORPUS:; SetErr catches the runtime error.  The
; program runs through inject with guard (runwrap=): a pattern behind
; every AllocMem, checked at FreeMem and at the end.
; Correct: LoadShape refuses the file ("Illegal Shape Size", caught: the
; handler checks that the shape stays empty and ends the program), nothing
; is written behind an allocation.  The original writes the BODY behind
; the block of 0 bytes until the data of the file ends ("Can't Read File"
; of ilbmifflib): the shape width stays 0 as in the correct case, so only
; inject's report of the write behind the allocation of 8 bytes shows it
; (the deviation of libs=cd).
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

!TestCase{width_zero}
  ; setup: the ILBM in memory, written to CORPUS:
  n=4
  m=AllocMem(n+256,65537) : p=0
  pl{$464F524D} : pl{0} : pl{$494C424D}
  pl{$424D4844} : pl{20}
  pw{0} : pw{2} : pw{0} : pw{0} : pb{1} : pb{0} : pb{0} : pb{0}
  pw{0} : pb{10} : pb{11} : pw{320} : pw{200}
  pl{$424F4459} : pl{n}
  p=p+n
  Poke.l m+4,p-8
  ok=WriteFile(0,"CORPUS:bo1054w.iff")
  !Require{ok,"file_written"}
  WriteMem 0,m,p
  CloseFile 0
  FreeMem m,n+256
  SetErr
    KillFile "CORPUS:bo1054w.iff"
    !CheckEq{Peek.w(Addr Shape(0)),0,"shape_width"}
    End
  End SetErr
  ; act
  LoadShape 0,"CORPUS:bo1054w.iff"
  !Check{0,"not_refused"}
!EndTestCase
