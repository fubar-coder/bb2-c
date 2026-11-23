; model-run-program: a case starts a program with !RunProgram and checks
; its return code and output in BASIC (ctest blitz2_selftest_model_run_program,
; change basic-test-local-data task 3.6), with the three compilers: a
; command line that runs a child of two lines and returns 20, a child that
; prints 3000 bytes (printed holds the first 1024), two runs in one case
; (the one result shows the second), a child whose runtime error shows a
; requester (the case's requester in the configuration) and a child that
; prints a line and hangs (its run open, its output kept, the session's
; end declared).
!TestCase{returns}
  *r.TLRun=!RunProgram{"@TESTDIR@/two"+Chr$(10)+"Quit 20","two"}
  !CheckEq{*r\rc,20,"rc"}
  !CheckEq{*r\rcknown,-1,"rcknown"}
  !CheckEq{*r\cut,0,"cut"}
  !CheckStrEq{*r\printed,"one"+Chr$(10)+"two"+Chr$(10)+"Quit failed returncode 20"+Chr$(10),"printed"}
!EndTestCase

!TestCase{long}
  *r.TLRun=!RunProgram{"@TESTDIR@/many","many"}
  !CheckEq{*r\rc,0,"rc"}
  !CheckEq{*r\cut,-1,"cut"}
  !CheckEq{*r\size,3000,"size"}
  !CheckEq{Len(*r\printed),1024,"length"}
  !CheckStrEq{Left$(*r\printed,30),"line 0 xxxxxxxxxxxxxxxxxxxxxx"+Chr$(10),"start"}
!EndTestCase

!TestCase{twice}
  *a.TLRun=!RunProgram{"Echo alpha","alpha"}
  x$=*a\printed
  *b.TLRun=!RunProgram{"Echo beta","beta"}
  !CheckStrEq{x$,"alpha"+Chr$(10),"copy"}
  !CheckStrEq{*a\printed,"beta"+Chr$(10),"overwritten"}
  !CheckEq{*a,*b,"one_block"}
!EndTestCase

!TestCase{req}
  *r.TLRun=!RunProgram{"@TESTDIR@/req","req"}
  !CheckStrEq{*r\printed,"req"+Chr$(10),"printed"}
!EndTestCase

!TestCase{hang}
  *r.TLRun=!RunProgram{"@TESTDIR@/hang","child"}
  !CheckEq{1,0,"after"}
!EndTestCase
