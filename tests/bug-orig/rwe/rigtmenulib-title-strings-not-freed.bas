; rigtmenulib-title-strings-not-freed (#695) - GTMenuTitle copies the
; title text with maxslib's AllocSt; GTFreeMenu of a title (and the
; object's free routine) frees the title node and its items but never
; the title string: each removed title loses its copy until the end of
; the program.  The program adds and removes a title with a 1000
; character text 200 times and compares the free memory before and
; after.
; Correct: the title strings are freed, the free memory stays about the
; same.
; @fixed #695
; @rwe rigtmenulib
DEFTYPE .l
t$=String$("x",1000)
GTMenuTitle 0,0,t$
GTFreeMenu 0,0
a=AvailMem_(0)
For k=1 To 200
  GTMenuTitle 0,0,t$
  GTFreeMenu 0,0
Next
b=AvailMem_(0)
If a-b>50000 Then NPrint "title strings lost" Else NPrint "title strings freed"
End
