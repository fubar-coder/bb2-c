; rigtmenulib-createmenustrip-unchecked (#689) - CreateMenuStrip builds
; the NewMenu array from the object's lists without checking anything:
; with no title in the list it takes the "title" at address 0 (its text
; is the long at 4, ExecBase; its items the list at the long at 8) and
; walks whatever it finds there; CreateMenusA and GetVisualInfoA are
; not checked and the result is -1 in every case.  Compiled without the
; debugger (whose check would report the empty list).
; Correct: CreateMenuStrip on a list without titles does nothing and
; returns 0.
; @rwe rigtmenulib
WbToScreen 0
NPrint "before"
r.l=CreateMenuStrip(0)
NPrint r
End
