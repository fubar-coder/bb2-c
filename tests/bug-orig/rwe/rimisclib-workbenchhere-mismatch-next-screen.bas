; rimisclib-workbenchhere-mismatch-next-screen (#760) - WorkbenchHere
; searches "Workbench" in the screen titles; a "W" starts a comparison,
; and on the first mismatch it goes on with the next screen instead of
; searching the rest of the title: "A Window on Workbench" is not found
; (the "W" of "Window" fails at "i").  The program opens a screen and
; gives every screen the title "A Window on Workbench" for the call (and
; restores them).  In the original no screen is found, and as it never
; ends the screen list (#761) it does not return (the session ends in a
; timeout).
; WorkbenchHere takes IntuitionBase from d0 (#759): the program loads it
; with GetReg.
; Correct: the rest of the title is searched, WorkbenchHere finds
; "Workbench" and returns -1.
; @fixed #760
; @rwe rimisclib
; @pass timeout=30
DEFTYPE .l
Dim t.l(20),sc.l(20)
Screen 0,0,0,320,200,2,0,"Other",1,2
ib=OpenLibrary_("intuition.library",0)
s=AllocMem_(32,$10001)
Poke$ s,"A Window on Workbench"
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
FreeMem_ s,32
CloseLibrary_ ib
NPrint r
End
