; model-expect-error: an announced runtime error judged like a check (change
; basic-test-runner-extension, section 6; ctest
; blitz2_selftest_model_expect_error and _originals). The bug of #1052
; (shapeslib-loadshape-depth-zero): LoadShape of an ILBM with 0 planes.
; The port refuses it with "Illegal Shape Depth", the announced error; the
; CD's libraries read the file and stop with "Can't Read File" of
; ilbmifflib, which model-expect-error.test.toml declares for libs=cd: the
; announcement fails there and the requester is the other one. Both sides
; pass. model-expect-error-removed.test.bas is the same source without the
; deviation: the side originals fails naming the announcement.
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
  ; setup: the ILBM in memory, written to CORPUS:
  n=4
  m=AllocMem(n+256,65537) : p=0
  pl{$464F524D} : pl{0} : pl{$494C424D}
  pl{$424D4844} : pl{20}
  pw{16} : pw{2} : pw{0} : pw{0} : pb{0} : pb{0} : pb{0} : pb{0}
  pw{0} : pb{10} : pb{11} : pw{16} : pw{2}
  pl{$424F4459} : pl{n}
  p=p+n
  Poke.l m+4,p-8
  ok=WriteFile(0,"CORPUS:me1052.iff")
  !Require{ok,"file_written"}
  WriteMem 0,m,p
  CloseFile 0
  FreeMem m,n+256
  ; act
  !ExpectError{"Illegal Shape Depth","refused"}
  LoadShape 0,"CORPUS:me1052.iff"
  !Check{0,"not_refused"}
!EndTestCase
