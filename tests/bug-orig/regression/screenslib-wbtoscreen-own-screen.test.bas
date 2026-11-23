; screenslib-wbtoscreen-own-screen - a port bug, not a bug of the
; original: WbToScreen frees the object's old screen first (closes it if
; the library opened it) and then looks for the Workbench screen from
; IntuitionBase's FirstScreen. The port read FirstScreen before the free,
; so when the closed screen was the front one it walked the freed screen
; (a crash). The original reads the list after it.
; Screen 0 (in front), then WbToScreen 0 on the same object.
; Correct (what the original does): the Workbench is found.

DEFTYPE .l

!TestCase{wbtoscreen_front_screen}
  Screen 0,0,0,320,200,2,0,"Own",1,2
  WbToScreen 0
  s=Peek.l(Addr Screen(0))
  !Check{(Peek.w(s+20)&15)=1,"workbench_found"}
!EndTestCase
