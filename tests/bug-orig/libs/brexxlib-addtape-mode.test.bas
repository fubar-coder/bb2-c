; brexxlib-addtape-mode (#96) - appending an event clears the mode word:
; addtape sets mode to $8000 for an executed event (the input handler
; then clears bit 15, mode is 0) and clears it for a trapped one.  Every
; Type, AbsMouse, RelMouse, MouseButton or ClickButton ends a running
; Record or PlayBack.
; Here: Record 0, then an executed AbsMouse while it records.
; Correct: XStatus stays 1 (recording).  The original prints 0.
DEFTYPE .l

!TestCase{record_abs_mouse}
  Record 0
  VWait 10
  AbsMouse 40,30
  VWait 10
  !CheckEq{XStatus,1,"status_recording"}
  Record
  !CheckEq{XStatus,0,"status_stopped"}
!EndTestCase
