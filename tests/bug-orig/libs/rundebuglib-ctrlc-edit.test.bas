; rundebuglib-ctrlc-edit (#43) - as rundebuglib-ctrlc-console, for the
; input: an executable compiled with runtime error checking cannot be
; stopped with Ctrl-C.  Each case starts a child (rundebuglib-ctrlc-edit.test/
; child-s.bas, child-f.bas) with its input from a file of one line
; (input.txt); the child sends itself Ctrl-C (exec SetSignal of
; SIGBREAKF_CTRL_C) and reads a line with Edit$ (child-s) or a number with
; Edit (child-f) from the default input, clears the signal and prints
; "not stopped".
; Correct (as a C program that reads its standard input): Edit$ ends the
; child with "***Break" and the return code 20; the child of Edit too.
!TestCase{edit_str}
  *r.TLRun=!RunProgram{"@TESTDIR@/child-s <@TESTDIR@/input.txt","child"}
  !CheckEq{*r\rc,20,"rc"}
  w$="***Break"+Chr$(10)+"@TESTDIR@/child-s failed returncode 20"+Chr$(10)
  !CheckStrEq{*r\printed,w$,"printed"}
!EndTestCase

!TestCase{edit_num}
  *r.TLRun=!RunProgram{"@TESTDIR@/child-f <@TESTDIR@/input.txt","child"}
  !CheckEq{*r\rc,20,"rc"}
  w$="***Break"+Chr$(10)+"@TESTDIR@/child-f failed returncode 20"+Chr$(10)
  !CheckStrEq{*r\printed,w$,"printed"}
!EndTestCase
