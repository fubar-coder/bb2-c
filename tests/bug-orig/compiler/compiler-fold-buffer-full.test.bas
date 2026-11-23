; compiler-fold-buffer-full (#1190) - when both operands of an operator
; are constants, blitz2 folds the operation: it writes MOVE.L Dn,D0 and
; RTS behind the code of the operands and runs that code in its own
; process (eval5, blitz2.s:9017).  It looks at nomemleft (the object
; buffer is full) before it writes the two instructions, not after:
; when the operands end at the end of the buffer or 2 bytes before it,
; the RTS is not written and the compiler runs on into the library area
; behind the buffer, into the first library copied there.  What that
; code does depends on the library: here Dim copies its library first,
; and the compiler crashes (the emulated CPU halts).
; The object buffer of the test is 2992 bytes (jobline objbuf=): the
; operands of a fold near the last line of the child
; (compiler-fold-buffer-full.test/child.bas) end there.  The compile of
; the child is the observation (the query data.child.bas); the child is
; compiled without the driver, whose code would change where the buffer
; ends.  With "Make Smallest Code" of the default options blitz2 sizes
; the buffer to the code after the first pass and compiles again (no "Not
; Enough Object Memory").
; Correct: the child compiles and prints 12.
DEFTYPE .l

!TestCase{child}
  *r.TLRun=!RunProgram{"@TESTDIR@/child","child"}
  !CheckEq{*r\rc,0,"rc"}
  !CheckStrEq{*r\printed,"12"+Chr$(10),"printed"}
!EndTestCase
