; rundebuglib-supertrap-d0-lost (#246) - a runtime error (TRAP #0, d0 =
; message) in supervisor mode: mytrap0 continues at more2, which calls
; codefinit (fixtraps, CloseLibrary) and then gives TRAP #0 to the old
; vector - with d0 changed by codefinit.
; Here the program sets its own TRAP #0 handler, runs itself a second
; time (own name from dos GetProgramName, dos Execute, output to the same
; file) and puts the vector back.  The child makes a runtime error
; (ShapeWidth of an unused shape: "Uninitialized Shape") once, inside a
; SetInt 5 routine (supervisor mode); the handler of the parent keeps d0
; and returns.  The original gives d0 = 0, and more2 runs on behind its
; trap.  (Since #1069 a SetInt routine is the only place where a runtime
; error in supervisor mode reaches rundebuglib.  Since #1073 rundebuglib
; gives such an error to no old vector: it leaves the routine and shows
; the message at the child's next statement, the parent's handler is not
; called.)
; Correct: the message ("Uninitialized Shape") is kept: the child's
; requester shows it, and the child ends there.
; @fixed #246
; @configs clib
; @pass runtime_errors=on
DEFTYPE .l
If NumPars=0
  v=cr_vbr
  o=Peek.l(v+128)
  Poke.l ?ar,0
  nm$=String$(Chr$(0),256) : GetProgramName_ &nm$,255
  c$=Peek$(&nm$)+" 1"
  Poke.l v+128,?th
  Execute_ &c$,0,Output_
  Poke.l v+128,o
  d=Peek.l(?ar)
  If d<>0 AND (d AND 1)=0
    NPrint "message ",Peeks$(d,19)="Uninitialized Shape"
  Else
    NPrint "message ",d
  EndIf
  End
EndIf
NPrint "child"
SetInt 5
  If f=0
    f=1
    w=ShapeWidth(5)
  EndIf
End SetInt
For i=1 To 10 : VWait : Next
NPrint "child back"
End

th:
  LEA ar(pc),a0 : MOVE.l d0,(a0)
  RTE

ar:
  Dc.l 0
