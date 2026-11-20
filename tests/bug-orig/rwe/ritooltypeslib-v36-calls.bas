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
; FreeIconObject.  The results are printed together at the end, so that
; the original shows the same in every memory layout.
; Correct: on every Kickstart: "icon", "<T005=1> <1>", -1, "icon",
; "<>", the new tool type "<NEW=1>", -1.
; @fixed #1149
; @rwe ritooltypeslib
; @machine a500 a500-204 a1200
; @pass timeout=30
; @pass file=one.info|@TESTDATA@/ritooltypeslib-tooltypes-9.info
; @pass file=none.info|@TESTDATA@/ritooltypeslib-v36-calls-none.info
DEFTYPE .l
NPrint "before"
n$=Chr$(10)
If GetIconObject("CORPUS:one") Then r$="icon" Else r$="no icon"
r$=r$+n$+"<"+FindToolNumber(4)+"> <"+FindToolValue("T009")+">"
r$=r$+n$+Str$(FreeIconObject)
If GetIconObject("CORPUS:none") Then r$=r$+n$+"icon" Else r$=r$+n$+"no icon"
r$=r$+n$+"<"+FindToolNumber(0)+">"
r=NewToolType("NEW","1")
r$=r$+n$+"<"+FindToolNumber(0)+">"
r$=r$+n$+Str$(FreeIconObject)
NPrint r$
End
