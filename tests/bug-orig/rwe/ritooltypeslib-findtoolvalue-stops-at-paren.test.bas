; ritooltypeslib-findtoolvalue-stops-at-paren (#844) - FindToolValue
; copies the value up to the end or the first ")": the ")" is meant for
; the end of a switched-off entry "(NAME=value)", but a ")" inside a value
; ends it too.  The icon is the one of SYS:Prefs with its tool types
; replaced by PAREN=a)b and (OFFV=x); "(OFFV" is the name icon.library
; finds the switched-off entry with.
; Correct: PAREN gives "a)b"; the closing ")" of "(OFFV=x)" is still
; dropped: "x".
DEFTYPE .l

!TestCase{paren_value}
  opened=GetIconObject("SYS:Prefs")
  !Require{opened<>0,"icon"}
  ClearToolTypes
  r=NewToolType("PAREN","a)b")
  r=NewToolType("OFFV","x",0)
  p$=FindToolValue("PAREN")
  o$=FindToolValue("(OFFV")
  !CheckStrEq{p$,"a)b","paren_value"}
  !CheckStrEq{o$,"x","off_value"}
!EndTestCase
