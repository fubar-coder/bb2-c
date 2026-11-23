; ritooltypeslib-empty-name-copies-on (#841) - SetToolValue/NewToolType
; build the new entry with a loop that copies a byte of the name first
; and tests the next byte afterwards: for an empty name it copies the
; name's 0 and the bytes behind it.  The entry begins with 0: the new
; tool type reads back as "".  The icon is the one of SYS:Prefs, its tool
; types cleared first.
; Correct: the entry is the name (nothing) and "=" and the value: "=abc".
DEFTYPE .l

!TestCase{empty_name}
  opened=GetIconObject("SYS:Prefs")
  !Require{opened<>0,"icon"}
  ClearToolTypes
  r=NewToolType("","abc")
  e$=FindToolNumber(0)
  !CheckEq{r,-1,"added"}
  !CheckStrEq{e$,"=abc","entry"}
!EndTestCase
