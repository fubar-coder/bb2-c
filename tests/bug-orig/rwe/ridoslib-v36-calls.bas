; ridoslib-v36-calls (#1142) - the library's init opens dos.library V36
; and then calls CreateMsgPort (exec V36) whether or not it opened; on
; Kickstart 1.3 (exec V34) the offset lies behind exec's jump table: every
; program with the library crashes at start-up, before its first line
; (the session ends in a timeout).  GetDeviceList calls LockDosList (dos
; V36) without testing the V36 base either.
; Correct: the program runs on Kickstart 1.3; the library needs Kickstart
; 2.0 for notification and the DOS list, so AddNotify, NotifyPort,
; CheckNotify and GetDeviceList return 0, NextDevice "", DeviceEntryType
; -1, RemNotify and FreeDeviceList do nothing; GetFileName works.  Only
; the a500: from Kickstart 2.0 on the results differ (the other tests of
; the library cover them).
; @fixed #1142
; @rwe ridoslib
; @machine a500
; @pass timeout=30
DEFTYPE .l
NPrint "before"
NPrint "AddNotify ",AddNotify(1,"RAM:ridoslib-v36")
NPrint "NotifyPort ",NotifyPort
NPrint "CheckNotify ",CheckNotify
RemNotify 1
NPrint "GetDeviceList ",GetDeviceList(16)
NPrint "NextDevice [",NextDevice,"]"
NPrint "DeviceEntryType ",DeviceEntryType
FreeDeviceList
NPrint "GetFileName ",GetFileName("RAM:dir/file")
NPrint "after"
End
