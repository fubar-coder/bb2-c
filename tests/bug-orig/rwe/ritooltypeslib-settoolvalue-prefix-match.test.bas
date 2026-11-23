; ritooltypeslib-settoolvalue-prefix-match (#853) - SetToolValue looks
; for the entry to replace by comparing the name up to its end only: an
; entry that just begins with the name matches.  The icon is the one of
; SYS:Prefs with its tool types replaced by FLAG and FL=1;
; SetToolValue "FL","x" replaces FLAG, the first entry that begins with
; "FL".
; Correct: FL=1 becomes FL=x, FLAG stays.
DEFTYPE .l

!TestCase{prefix}
  opened=GetIconObject("SYS:Prefs")
  !Require{opened<>0,"icon"}
  ClearToolTypes
  r=NewToolType("FLAG","")
  r=NewToolType("FL","1")
  s=SetToolValue("FL","x")
  e0$=FindToolNumber(0)
  e1$=FindToolNumber(1)
  !CheckEq{s,-1,"set"}
  !CheckStrEq{e0$,"FLAG","first_entry"}
  !CheckStrEq{e1$,"FL=x","second_entry"}
!EndTestCase
