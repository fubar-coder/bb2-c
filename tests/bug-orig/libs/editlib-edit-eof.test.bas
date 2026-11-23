; editlib-edit-eof (#132) - at the end of the file the default input
; (DefaultInput: dos Read(Input(),buffer,n+1)-1) returns -1 characters;
; Edit$ adds that to a3 unchecked and returns the length -1 (a3 one byte
; back), Edit gives Val the length -1.  The case starts a child
; (editlib-edit-eof.test/child.bas) with the input from NIL: (as the
; corpus test t41 does with a file); the child prints the length of
; Edit$(10), of Edit$("ab",5) and the value of Edit(5).
; Correct: at the end of the file Edit$ is the empty string and Edit 0.
!TestCase{at_end}
  *r.TLRun=!RunProgram{"@TESTDIR@/child <NIL:","child"}
  !CheckEq{*r\rc,0,"rc"}
  !CheckStrEq{*r\printed,"edit$ 0"+Chr$(10)+"edit$dflt 0"+Chr$(10)+"edit 0"+Chr$(10),"printed"}
!EndTestCase
