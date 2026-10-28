; windowslib-titel-leck (#318) - Window, WindowTags and WTitle copy the
; titles into new memory (maxslib allocst) and never free the old copy:
; every new title costs memory until the program ends.
; On the Workbench screen: 100 times WTitle with a window and a screen
; title of 1000 characters each (200000 bytes in copies), free memory
; (exec AvailMem) measured before and after.
; Correct: the free memory does not shrink by more than 50000 bytes.
; @configs clib
DEFTYPE .l
WbToScreen 0
Window 0,20,30,200,100,$000E,"Leak",1,2
DefaultOutput
t$=String$("t",1000) : s$=String$("s",1000)
MOVE.l 4,a6 : MOVEQ #0,d1 : JSR -216(a6) : PutReg d0,m1
For i=1 To 100
  WTitle t$,s$
Next
MOVE.l 4,a6 : MOVEQ #0,d1 : JSR -216(a6) : PutReg d0,m2
NPrint "lost under 50000 ",m1-m2<50000
CloseWindow 0
End
