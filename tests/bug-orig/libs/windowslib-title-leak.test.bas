; windowslib-title-leak (#318) - Window, WindowTags and WTitle copy the
; titles into new memory (maxslib allocst) and never free the old copy:
; every new title costs memory until the program ends.
; Here: on the Workbench screen: 100 times WTitle with a window and a
; screen title of 1000 characters each (200000 bytes in copies), then 100
; times Window and CloseWindow, then 100 times WindowTags and CloseWindow,
; each with a window title of 1000 characters (100000 bytes each); free
; memory (exec AvailMem) measured before and after each part.
; Correct: the free memory does not shrink by more than 50000 bytes in
; any part.
DEFTYPE .l

!TestCase{lost_memory}
  WbToScreen 0
  Window 0,20,30,200,100,$000E,"Leak",1,2
  t$=String$("t",1000) : s$=String$("s",1000)
  MOVE.l 4,a6 : MOVEQ #0,d1 : JSR -216(a6) : PutReg d0,m1
  For i=1 To 100
    WTitle t$,s$
  Next
  MOVE.l 4,a6 : MOVEQ #0,d1 : JSR -216(a6) : PutReg d0,m2
  !Check{m1-m2<50000,"wtitle_under_limit"}
  CloseWindow 0
  MOVE.l 4,a6 : MOVEQ #0,d1 : JSR -216(a6) : PutReg d0,m1
  For i=1 To 100
    Window 1,20,30,200,100,$000E,t$,1,2
    CloseWindow 1
  Next
  MOVE.l 4,a6 : MOVEQ #0,d1 : JSR -216(a6) : PutReg d0,m2
  !Check{m1-m2<50000,"window_under_limit"}
  MOVE.l 4,a6 : MOVEQ #0,d1 : JSR -216(a6) : PutReg d0,m1
  For i=1 To 100
    WindowTags 1,$000E,t$
    CloseWindow 1
  Next
  MOVE.l 4,a6 : MOVEQ #0,d1 : JSR -216(a6) : PutReg d0,m2
  !Check{m1-m2<50000,"windowtags_under_limit"}
!EndTestCase
