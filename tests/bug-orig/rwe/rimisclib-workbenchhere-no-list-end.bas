; rimisclib-workbenchhere-no-list-end (#761) - WorkbenchHere goes to the
; next screen without testing for the end of the list: after the last
; screen it reads the "screen" at address 0 (its title pointer at 22, the
; next screen at 0) and loops for ever (the session ends in a timeout),
; or returns -1 if the bytes there contain "Workbench".  The emulated
; system has no Workbench screen open; the program opens a screen
; "Other" and gives every screen the title "Other" for the call (and
; restores them).
; WorkbenchHere takes IntuitionBase from d0 (#759): the program loads it
; with GetReg.
; Correct: no screen title contains "Workbench", WorkbenchHere returns 0.
; @rwe rimisclib
DEFTYPE .l
Dim t.l(20),sc.l(20)
Screen 0,0,0,320,200,2,0,"Other",1,2
ib=OpenLibrary_("intuition.library",0)
s=AllocMem_(16,$10001)
Poke$ s,"Other"
n=0
x=Peek.l(ib+60)
While x<>0 AND n<20
  sc(n)=x : t(n)=Peek.l(x+22) : Poke.l x+22,s
  n+1 : x=Peek.l(x)
Wend
NPrint "before"
GetReg d0,ib
r=WorkbenchHere
For i=0 To n-1 : Poke.l sc(i)+22,t(i) : Next
FreeMem_ s,16
CloseLibrary_ ib
NPrint r
End
