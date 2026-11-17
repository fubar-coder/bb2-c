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
; the end (34 allocations of 45012 bytes are those of the program's
; environment).
; @fixed #855
; @rwe ritooltypeslib
; @pass runwrap=@INJECT@|allocfail=88-88/1 track
; @pass file=few.info|@TESTDATA@/ritooltypeslib-tooltypes-9.info
DEFTYPE .l
If GetIconObject("CORPUS:few")=0 Then NPrint "no icon" : End
NPrint NewToolType("NEW","1")
NPrint "<",FindToolNumber(8),"> <",FindToolNumber(9),">"
NPrint NewToolType("NEW","1")
NPrint "<",FindToolNumber(9),"> <",FindToolNumber(10),">"
r=FreeIconObject
End
