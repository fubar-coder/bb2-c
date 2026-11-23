; riapplib-delappwindow-empty (#523) - DelAppWindow does not test its
; slot: for a window number without an App window it calls
; RemoveAppWindow with the empty slot (NULL), clears it and returns -1
; (success); DelAppMenu and DelAppIcon return 0 for an empty slot.  The
; program calls DelAppWindow(3) without an AddAppWindow before.
; Correct: there is no App window 3, DelAppWindow returns 0.
DEFTYPE .l

!TestCase{empty_slot}
  r=DelAppWindow(3)
  !CheckEq{r,0,"result"}
!EndTestCase
