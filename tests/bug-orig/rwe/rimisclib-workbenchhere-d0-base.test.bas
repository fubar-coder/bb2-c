; rimisclib-workbenchhere-d0-base (#759) - WorkbenchHere loads the
; library's IntuitionBase and overwrites it with d0 (MOVE.l intuibase,a0 :
; MOVE.l d0,a0); the function has no arguments, so it searches the screen
; list of whatever the program left in d0.  The program builds a fake
; IntuitionBase whose only screen has the title "Workbench" and leaves its
; address in d0 (GetReg); the real screens are all titled "Other" for
; the call (and restored).  The original searches the fake list and
; returns -1.
; Correct: WorkbenchHere searches the system's screens, none contains
; "Workbench": 0.
DEFTYPE .l

!TestCase{fake_base}
  Dim t.l(20),sc.l(20)
  Screen 0,0,0,320,200,2,0,"Other",1,2
  ib=OpenLibrary_("intuition.library",0)
  s=AllocMem_(16,$10001)
  Poke$ s,"Other"
  w=AllocMem_(16,$10001)
  Poke$ w,"Workbench"
  fs=AllocMem_(64,$10001)
  Poke.l fs+22,w
  fb=AllocMem_(64,$10001)
  Poke.l fb+60,fs
  n=0
  x=Peek.l(ib+60)
  While x<>0 AND n<20
    sc(n)=x : t(n)=Peek.l(x+22) : Poke.l x+22,s
    n+1 : x=Peek.l(x)
  Wend
  GetReg d0,fb
  r=WorkbenchHere
  For i=0 To n-1 : Poke.l sc(i)+22,t(i) : Next
  FreeMem_ fb,64 : FreeMem_ fs,64 : FreeMem_ w,16 : FreeMem_ s,16
  CloseLibrary_ ib
  !CheckEq{r,0,"result"}
!EndTestCase
