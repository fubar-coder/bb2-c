; screenslib-findscreen-closes-owner (#248) - FindScreen and WbToScreen
; free the object's old screen first, and freescreen closes every screen
; with "bLtZ" in UserData: also one the object only found, which another
; object opened (Screen). CloseScreen of such an object closes it too.
; The opening object then points to a closed screen.
; Screen 0 opens "Blitz"; FindScreen 1,"Blitz" finds it; WbToScreen 1;
; is object 0's screen still in IntuitionBase's screen list? Then the
; same with Screen 3 "Other", FindScreen 2,"Other" and CloseScreen 2.
; Correct: both times the opening object's screen is still open (-1).
DEFTYPE .l

; is the screen s0 in the screen list of IntuitionBase ib?
Function.l inlist{ib,s0}
  f=0 : s=Peek.l(ib+60)
  While s<>0
    If s=s0 Then f=-1
    s=Peek.l(s)
  Wend
  Function Return f
End Function

!TestCase{owner_kept}
  Dim nm.l(8) : Poke.s &nm(0),"intuition.library"
  GetReg d0,&nm(0) : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -408(a6) : PutReg d0,ib
  !Require{ib<>0,"intuitionbase"}
  Screen 0,0,0,320,200,2,0,"Blitz",1,2
  s0=Peek.l(Addr Screen(0))
  FindScreen 1,"Blitz"
  !Check{Peek.l(Addr Screen(1))=s0,"found"}
  WbToScreen 1
  f=inlist{ib,s0}
  !CheckEq{f,-1,"after_wbtoscreen"}
  If f Then CloseScreen 0 Else Poke.l Addr Screen(0),0
  Screen 3,0,0,320,200,2,0,"Other",1,2
  s0=Peek.l(Addr Screen(3))
  FindScreen 2,"Other"
  CloseScreen 2
  f=inlist{ib,s0}
  !CheckEq{f,-1,"after_closescreen"}
  If f Then CloseScreen 3 Else Poke.l Addr Screen(3),0
!EndTestCase
