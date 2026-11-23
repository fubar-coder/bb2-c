; debuglib-setint-runtime-error (#1069) - a port regression test: a
; runtime error inside a SetInt routine of a program run from the editor
; with the debugger.  The routine runs in supervisor mode, where exec does
; not pass a trap to the task's trap handler: since #1069 debuglib takes
; the traps through that handler, and runerrlib's gosuper and gouser
; make the CPU's trap vectors lead to it while the routine runs.  The
; driver runs from the editor (editor-run, tedstub's COMPILE/RUN) with
; runtime error checking on, without a debugger program: the debugger's
; default handler of an error in an interrupt routine ends the program at
; its next statement.  The routine calls a Statement (TRAP #4/#5 in
; supervisor mode too) and counts; the case waits 100 frames.
; rundebuglib's handling of such an error (the executable from the shell)
; has its own test (rundebuglib-setint-runtime-error).
; Expected: the routine's runtime error at the count of 20 ends the
; program before the case gets behind its wait; the session goes on.
DEFTYPE .l
Statement inc{}
  SHARED c
  c+1
End Statement

!TestCase{setint_error}
  Dim a(5)
  SetInt 5
    inc{}
    If c=20 Then a(c)=1
  End SetInt
  NPrint "start"
  For i=1 To 100 : VWait : Next
  !Check{0,"behind_wait"}
!EndTestCase
