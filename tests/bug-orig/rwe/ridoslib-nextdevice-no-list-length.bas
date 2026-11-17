; ridoslib-nextdevice-no-list-length (#625) - NextDevice is a string
; function (result at a3, its length in d0).  Without a locked list (no
; GetDeviceList, or after FreeDeviceList) it returns at once without
; setting d0: the result's length is whatever d0 held on entry, the
; characters whatever the string buffer holds there.  Here d0 holds the
; 3 that the assignment before the call left in it.
; Correct: NextDevice without a locked list returns "" (length 0), before
; any GetDeviceList and after FreeDeviceList.
; @fixed #625
; @rwe ridoslib
; @pass timeout=30
DEFTYPE .l
n=3
a$=NextDevice
NPrint "before GetDeviceList: length ",Len(a$)
If GetDeviceList(8)
  FreeDeviceList
EndIf
n=3
a$=NextDevice
NPrint "after FreeDeviceList: length ",Len(a$)
End
