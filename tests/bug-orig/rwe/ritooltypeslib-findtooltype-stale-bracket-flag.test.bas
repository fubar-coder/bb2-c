; ritooltypeslib-findtooltype-stale-bracket-flag (#843) - FindToolType
; tt$,1 sets a flag for the bracket search ("(tt$)", a switched-off tool
; type) that only the end of that search clears.  After a call that
; returns before it (here: FOO is found without brackets) the flag stays
; set, and the next one-argument FindToolType also searches "(tt$)".
; The icon is the one of SYS:Prefs with its tool types replaced by FOO=1
; and (BRACK).
; Correct: FindToolType("BRACK") does not find "(BRACK)": 0 both times.
DEFTYPE .l

!TestCase{stale_flag}
  opened=GetIconObject("SYS:Prefs")
  !Require{opened<>0,"icon"}
  ClearToolTypes
  r=NewToolType("FOO","1")
  r=NewToolType("BRACK","",0)
  e$=FindToolNumber(1)
  f=FindToolType("FOO",1)
  b1=FindToolType("BRACK")
  b2=FindToolType("BRACK")
  !CheckStrEq{e$,"(BRACK)","entry"}
  !CheckEq{f,-1,"foo_found"}
  !CheckEq{b1,0,"brack_first"}
  !CheckEq{b2,0,"brack_second"}
!EndTestCase
