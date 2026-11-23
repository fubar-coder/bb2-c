; model-expect-error-removed: the source of model-expect-error.test.bas whose
; configuration declares no deviation (ctest
; blitz2_selftest_model_expect_error_removed, the check with broken input of
; section 6 of basic-test-runner-extension): with the CD's libraries the
; announced "Illegal Shape Depth" does not come and "Can't Read File" is a
; requester nobody announced, so the case fails naming the announcement.
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
