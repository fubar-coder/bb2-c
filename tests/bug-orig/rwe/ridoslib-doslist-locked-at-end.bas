; ridoslib-doslist-locked-at-end (#1081) - GetDeviceList locks the DOS
; list (LockDosList), only FreeDeviceList unlocks it: finit does not (the
; call is commented out in the original's _libfinit).  A program that
; ends without FreeDeviceList leaves the list locked for reading, and
; every later writer (Assign, a disk change, mounting) waits for ever.
; The program runs through inject: doslist tries to lock the list for
; writing after the program and releases a lock left behind (so that
; the session goes on).
; Correct: finit unlocks the list, it is free after the program.
; @fixed #1081
; @rwe ridoslib
; @pass runwrap=@INJECT@|doslist
; @pass timeout=60
If GetDeviceList(16)
  a$=NextDevice
  NPrint "locked"
EndIf
End
