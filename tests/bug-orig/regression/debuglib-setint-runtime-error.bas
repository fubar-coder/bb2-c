; debuglib-setint-runtime-error (#1069) - a port regression test: a
; runtime error inside a SetInt routine of a program run from the editor
; with the debugger.  The routine runs in supervisor mode, where exec does
; not pass a trap to the task's trap handler: since #1069 debuglib takes
; the traps through that handler, and runerrlib's gosuper and gouser
; make the CPU's trap vectors lead to it while the routine runs.  tedstub
; runs the program with COMPILE/RUN, runtime error checking on, without a
; debugger program: the debugger's default handler of an error in an
; interrupt routine ends the program at its next statement.  The routine
; calls a Statement (TRAP #4/#5 in supervisor mode too) and counts; the
; main program waits 100 frames.  The executable (the runner's own run,
; with an argument) only prints a line: rundebuglib stops the machine at
; such an error.
; Expected: the routine's runtime error at the count of 20 ends the
; program before it prints the count; the session goes on.
; @pass runtime_errors=on
; @pass runargs=exe
; @pass jobline=run=r001|CORPUS:src/debuglib-setint-runtime-error.bb2|RAM:con
DEFTYPE .l
If NumPars>0
  NPrint "executable"
  End
EndIf
Dim a(5)
Statement inc{}
  SHARED c
  c+1
End Statement
SetInt 5
  inc{}
  If c=20 Then a(c)=1
End SetInt
NPrint "start"
For i=1 To 100 : VWait : Next
NPrint "c ",c
End
