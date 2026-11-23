; compiler-incbin-odd-end (#979) - at the end of the program the compiler
; stores the address of the end code (endop) and emits the library finits
; and "moveq #0,d0 / rts" without aligning pc. An IncBin of odd length as
; the last code-generating statement leaves that code on an odd address:
; End jumps there (address error).
; Here the case starts a child (compiler-incbin-odd-end.test/child.bas):
; the driver's tail would be the last code-generating statement of a model
; test.  The 3 bytes of odd.bin come from odd.bin.spec.
; Correct: the child prints "before" and ends.
DEFTYPE .l

!TestCase{child}
  *r.TLRun=!RunProgram{"@TESTDIR@/child","child"}
  !CheckEq{*r\rc,0,"rc"}
  !CheckStrEq{*r\printed,"before"+Chr$(10),"printed"}
!EndTestCase
