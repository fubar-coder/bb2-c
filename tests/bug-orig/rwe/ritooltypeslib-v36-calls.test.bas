; ritooltypeslib-v36-calls (#1149) - GetIconObject reads the icon with
; GetDiskObjectNew (icon.library V36); the library opens icon.library in
; any version, and Workbench 1.3 has one (V34): on Kickstart 1.3 the call
; lies behind icon.library's jump table and runs whatever memory lies
; before the library (what it does depends on the memory layout; the
; machine crashes at the latest at FreeIconObject, the session ends in a
; timeout).  The same program on Kickstart 1.3, 2.04 and 3.1: the icon of
; a file with 9 tool types (test data ritooltypeslib-tooltypes-9.info),
; its fifth tool type and the value of T009, FreeIconObject; then an icon
; without tool types (test data ritooltypeslib-v36-calls-none.info), its
; first tool type, NewToolType on it and the tool type read back,
; FreeIconObject.  The results are collected and checked together at the
; end, so that the original shows the same in every memory layout.
; Correct: on every Kickstart: an icon, "T005=1", "1", -1, an icon, "",
; the new tool type "NEW=1", -1.
DEFTYPE .l

!TestCase{icons}
  o1=GetIconObject("CORPUS:one")<>0
  e4$=FindToolNumber(4)
  v9$=FindToolValue("T009")
  f1=FreeIconObject
  o2=GetIconObject("CORPUS:none")<>0
  n0$=FindToolNumber(0)
  r=NewToolType("NEW","1")
  n1$=FindToolNumber(0)
  f2=FreeIconObject
  !CheckEq{o1,-1,"first_icon"}
  !CheckStrEq{e4$,"T005=1","fifth_tooltype"}
  !CheckStrEq{v9$,"1","value"}
  !CheckEq{f1,-1,"first_free"}
  !CheckEq{o2,-1,"second_icon"}
  !CheckStrEq{n0$,"","no_tooltype"}
  !CheckStrEq{n1$,"NEW=1","new_tooltype"}
  !CheckEq{f2,-1,"second_free"}
!EndTestCase
