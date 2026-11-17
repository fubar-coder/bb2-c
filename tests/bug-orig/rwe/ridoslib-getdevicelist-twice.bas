; ridoslib-getdevicelist-twice (#1081) - GetDeviceList does not test
; whether it still holds the DOS list: a second GetDeviceList locks it
; again and overwrites the first lock, and one FreeDeviceList releases
; only one of the two.  The list stays locked for reading after the
; program, and every later writer (Assign, a disk change, mounting)
; waits for ever.  The program runs through inject: doslist tries to
; lock the list for writing after the program and releases a lock left
; behind (so that the session goes on).
; Correct: the second GetDeviceList releases the first lock, the list is
; free after FreeDeviceList.
; @fixed #1081
; @rwe ridoslib
; @pass runwrap=@INJECT@|doslist
; @pass timeout=60
DEFTYPE .l
a=GetDeviceList(16)
b=GetDeviceList(16)
FreeDeviceList
NPrint "results ",a," ",b
End
