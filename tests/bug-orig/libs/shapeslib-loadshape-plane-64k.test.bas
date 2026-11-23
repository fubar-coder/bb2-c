; shapeslib-loadshape-plane-64k (#263) - LoadShape keeps the bytes per
; plane of a shape in a word (+22) and allocates the data with that low
; word x depth: an ILBM with a plane of 64K or more gets a block that is
; far too small, and the decoder writes the whole BODY into it.
; Here: an ILBM of 1024x520 with 1 plane (66560 bytes per plane: the low
; word 1024), uncompressed, built in memory and written to CORPUS:; SetErr
; catches the runtime error.  The program runs through inject with guard
; (runwrap=): a pattern behind every AllocMem, checked at FreeMem and at
; the end.
; Correct: the shape format cannot hold the plane: LoadShape refuses the
; file ("Can't Read/Write Shapes File", caught: the handler checks that the
; shape stays empty and ends the program), nothing is written behind an
; allocation.  The original allocates 1024 bytes and writes 66560 into
; them: the shape has width 1024, inject reports the write behind the
; allocation of 1032 bytes (the deviations of libs=cd).
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

!TestCase{plane_64k}
  ; setup: the ILBM in memory, written to CORPUS:
  n=66560
  m=AllocMem(n+256,65537) : p=0
  pl{$464F524D} : pl{0} : pl{$494C424D}
  pl{$424D4844} : pl{20}
  pw{1024} : pw{520} : pw{0} : pw{0} : pb{1} : pb{0} : pb{0} : pb{0}
  pw{0} : pb{10} : pb{11} : pw{1024} : pw{520}
  pl{$424F4459} : pl{n}
  p=p+n
  Poke.l m+4,p-8
  ok=WriteFile(0,"CORPUS:bo263k.iff")
  !Require{ok,"file_written"}
  WriteMem 0,m,p
  CloseFile 0
  FreeMem m,n+256
  SetErr
    KillFile "CORPUS:bo263k.iff"
    !CheckEq{Peek.w(Addr Shape(0)),0,"shape_width"}
    End
  End SetErr
  ; act
  LoadShape 0,"CORPUS:bo263k.iff"
  !Check{0,"not_refused"}
!EndTestCase
