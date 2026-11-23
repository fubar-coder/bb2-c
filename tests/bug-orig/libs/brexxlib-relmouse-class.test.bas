; brexxlib-relmouse-class (#98) - RelMouse makes a POINTERPOS event
; (class 4) like AbsMouse, only with the qualifier RELATIVEMOUSE:
; movemouse writes the class 4 and ignores the class 2 (RAWMOUSE) that
; RelMouse passes.
; Here: RelMouse to a tape (the class of the event), then executed: the
; pointer is put to a position with AbsMouse, RelMouse 8,4 moves it.
; Correct: the event is RAWMOUSE (class 2) and the pointer moves by the
; offsets in mouse counts from where it was: 8 pixels right and 2 rows
; down on the Workbench screen (a count is a row of an interlaced
; screen).  The original's POINTERPOS event with RELATIVEMOUSE moves the
; pointer the same way under Kickstart 3.1; only its class is wrong.
DEFTYPE .l

!TestCase{rel_mouse_event}
  WbToScreen 0
  QuietTrap On
  TapeTrap 0
  RelMouse 8,4
  e=Peek.l(Addr Tape(0))
  !CheckEq{Peek.b(e+4),2,"event_class"}
  !CheckEq{Peek.w(e+8) AND $FFFF,$8000,"event_qualifier"}
  QuietTrap Off
  TapeTrap 1
  AbsMouse 200,100
  VWait 10
  x=SMouseX : y=SMouseY
  RelMouse 8,4
  VWait 10
  !CheckEq{SMouseX-x,8,"moved_x"}
  !CheckEq{SMouseY-y,2,"moved_y"}
!EndTestCase
