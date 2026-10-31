; rimisclib-workbenchhere-partial-at-title-end (#762) - WorkbenchHere
; searches "Workbench" in the screen titles; when a title ends during a
; partial match it restarts with the same screen: an endless loop (the
; session ends in a timeout).  The program opens a screen "Workbench"
; (the emulated system has no Workbench screen open) and in front of it
; a screen "My Work", which is searched first.  WorkbenchHere takes IntuitionBase from
; d0 (#759): the program loads it with GetReg.
; Correct: WorkbenchHere goes on with the next screen, finds "Workbench"
; and returns -1.
; @rwe rimisclib
; @pass timeout=30
DEFTYPE .l
Screen 1,0,0,320,200,2,0,"Workbench",1,2
Screen 0,0,0,320,200,2,0,"My Work",1,2
ib=OpenLibrary_("intuition.library",0)
NPrint "before"
GetReg d0,ib
r=WorkbenchHere
CloseLibrary_ ib
NPrint r
End
