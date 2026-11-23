; riapplib-window-range (#1097) - AddAppWindow and DelAppWindow use the
; window number as the index into the table of 16 AppWindows without a
; check; the AppIcon table lies behind it, so window 16 (a valid Blitz
; window with a larger Maximum) is the slot of App icon 0.  The original
; has no error check area.  Here with runtime error checking and
; DelAppWindow(16) (in the original: RemoveAppWindow with App icon 0's
; empty slot; the result is not checked: 0 in the port, -1 in the
; original, #523).
; Correct: the error check refuses the number: "App window number must
; be 0 to 15".
DEFTYPE .l

!TestCase{window_16}
  !ExpectError{"App window number must be 0 to 15","refused"}
  r=DelAppWindow(16)
  !Check{0,"not_refused"}
!EndTestCase
