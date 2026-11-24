; divzero-next-compile (#1215) - a compile that ends with "Division by
; Zero" leaves nothing behind for the next compile of the same compiler
; process: the data sources of divzero-next-compile.test/ are compiled in
; the cell's session after the driver, by the one compiler process of its
; job list, in their order.  div0.bas (a constant division on the plain
; path) and div0index.bas (a division of two Word constants in a later
; array index) end with "Division by Zero" (asserted in
; divzero-next-compile.test.toml); sum.bas, compiled after them, computes
; + - * / MOD on the plain path, + on the mixed path and a Word * in a
; later array index.  The case runs sum and checks what it prints.
; Correct: 7 12 -1 5 4 2 lt 42, one per line, what sum prints when it is
; compiled on its own.
!TestCase{after_div0}
  fh.l=Open_("RAM:divzero-next-compile.out",1006)
  !Require{fh<>0,"open_output"}
  c$="@TESTDIR@/sum"
  Execute_ &c$,0,fh
  Close_ fh
  fh=Open_("RAM:divzero-next-compile.out",1005)
  !Require{fh<>0,"open_back"}
  b$=String$(" ",200)
  n.l=Read_(fh,&b$,200)
  Close_ fh
  w$="7"+Chr$(10)+"12"+Chr$(10)+"-1"+Chr$(10)+"5"+Chr$(10)+"4"+Chr$(10)
  w$=w$+"2"+Chr$(10)+"lt"+Chr$(10)+"42"+Chr$(10)
  !CheckStrEq{Left$(b$,n),w$,"output"}
!EndTestCase
