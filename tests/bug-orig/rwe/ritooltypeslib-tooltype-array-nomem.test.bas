; ritooltypeslib-tooltype-array-nomem (#855) - the original keeps the tool
; types of the open icon in a static array of 100 entries, which
; NewToolType fills without a count (ritooltypeslib-tooltype-array-
; unbounded).  The fix allocates the array for the icon's tool types and
; makes it larger when NewToolType finds it full; this test is the case
; without memory for the larger array.  The icon (test data
; ritooltypeslib-tooltypes-9.info) has 9 tool types: an array of 10 entries
; (40 bytes); the first NewToolType needs one of 22 entries (88 bytes),
; whose allocation fails (inject allocfail).  The original needs no
; allocation there and adds the entry.
; Correct: the first NewToolType returns 0 and changes nothing, the second
; adds the entry; inject track finds no allocation of the library left at
; the end.
DEFTYPE .l

!TestCase{no_memory}
  d=GetIconObject("CORPUS:few")
  !Require{d<>0,"icon"}
  r1=NewToolType("NEW","1")
  e8a$=FindToolNumber(8)
  e9a$=FindToolNumber(9)
  r2=NewToolType("NEW","1")
  e9b$=FindToolNumber(9)
  e10b$=FindToolNumber(10)
  r=FreeIconObject
  !CheckEq{r1,0,"first_fails"}
  !CheckStrEq{e8a$,"T009=1","last_entry"}
  !CheckStrEq{e9a$,"","first_unchanged"}
  !CheckEq{r2,-1,"second_adds"}
  !CheckStrEq{e9b$,"NEW=1","new_entry"}
  !CheckStrEq{e10b$,"","end_entry"}
!EndTestCase
