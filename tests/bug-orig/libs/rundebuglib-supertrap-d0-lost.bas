; rundebuglib-supertrap-d0-lost (#246) - a runtime error (TRAP #0, d0 =
; message) in supervisor mode: mytrap0 continues at more2, which calls
; codefinit (fixtraps, CloseLibrary) and then gives TRAP #0 to the old
; vector - with d0 changed by codefinit.
; rundebuglib keeps the TRAP #0 vector it finds at start as the old one.
; Here the program (CORPUS:out/ID, where the runner puts it) sets its own
; TRAP #0 handler, runs itself a second time (dos Execute, output to the
; same file) and puts the vector back.  The child makes a runtime error
; (ShapeWidth of an unused shape: "Uninitialized Shape") inside exec
; Supervisor(); the handler of the parent keeps d0 and returns to the
; child's Supervisor() call with the stack pointer the child noted (the
; address of the parent's 2 longs comes as a decimal argument, parsed
; digit by digit: Val is a float).  The original gives d0 = 0.
; Correct: the old vector gets the message ("Uninitialized Shape").
; @configs clib
; @pass runtime_errors=on
DEFTYPE .l
If NumPars=0
  v=cr_vbr
  o=Peek.l(v+128)
  Poke.l ?ar,0
  c$="CORPUS:out/rundebuglib-supertrap-d0-lost "+Str$(?ar)
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
a$=Par$(1) : p=0
For i=1 To Len(a$) : p=p*10+Asc(Mid$(a$,i,1))-48 : Next
NPrint "child"
MOVE.l a5,a1 : LEA uf(pc),a5 : MOVE.l 4,a6 : JSR -30(a6)
NPrint "child back"
End

uf:
  MOVE.l a1,a5
  GetReg d0,p : MOVE.l d0,a0 : MOVE.l a7,4(a0)
  w=ShapeWidth(5)
  RTE

th:
  LEA ar(pc),a0 : MOVE.l d0,(a0) : MOVE.l 4(a0),a7
  RTE

ar:
  Dc.l 0,0
