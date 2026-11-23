; model-local-data: a test with data of its own, the directory
; model-local-data.test/ (ctest blitz2_selftest_model_local_data, change
; basic-test-local-data task 2.4): a copied file and one made by mktestdata
; that a case reads, and two data sources a case starts with dos Execute -
; child.bas compiled with each cell's build configuration, errs.bas with
; runtime error checking on in every cell (in the cell without it a compile
; part of its own).
!TestCase{files}
  fh.l=Open_("@TESTDIR@/input.txt",1005)
  !Require{fh<>0,"open_input"}
  b$=String$(" ",100)
  n.l=Read_(fh,&b$,100)
  Close_ fh
  !CheckStrEq{Left$(b$,n),"one line of input"+Chr$(10),"input"}
  fh=Open_("@TESTDIR@/bytes",1005)
  !Require{fh<>0,"open_bytes"}
  n=Read_(fh,&b$,100)
  Close_ fh
  !CheckStrEq{Left$(b$,n),"ABC"+Chr$(10),"bytes"}
!EndTestCase

!TestCase{children}
  fh.l=Open_("RAM:model-local-data.out",1006)
  !Require{fh<>0,"open_output"}
  c$="@TESTDIR@/child"
  Execute_ &c$,0,fh
  c$="@TESTDIR@/errs"
  Execute_ &c$,0,fh
  Close_ fh
  fh=Open_("RAM:model-local-data.out",1005)
  !Require{fh<>0,"open_back"}
  b$=String$(" ",200)
  n.l=Read_(fh,&b$,200)
  Close_ fh
  w$="child model-local-data CORPUS:model-local-data.test"+Chr$(10)+"errs"+Chr$(10)
  !CheckStrEq{Left$(b$,n),w$,"output"}
!EndTestCase
