; ridoslib-devicetype-after-free (#622) - FreeDeviceList unlocks the DOS
; list but leaves the current entry; DeviceEntryType reads dol_Type from
; it, also after the unlock, when the entry may have been removed and
; freed.  The program makes an assign RWEBUGORIG622 (AssignLock, RAM:),
; walks the assigns with NextDevice up to it, notes DeviceEntryType,
; calls FreeDeviceList and removes the assign (its DosList entry is
; freed).  The program runs through inject with freefill: every block the
; process frees is filled with $AA, so a read of the freed entry finds
; $AAAAAAAA.
; Correct: DeviceEntryType after FreeDeviceList returns the type of the
; last entry NextDevice gave (1, an assign) without reading the list.
; @fixed #622
; @rwe ridoslib
; @pass runwrap=@INJECT@|freefill=AA
DEFTYPE .l
n$="RWEBUGORIG622"
l=Lock_("RAM:",-2)
If AssignLock_(&n$,l)=0
  UnLock_ l
  NPrint "no assign"
  End
EndIf
t=-2
If GetDeviceList(16)
  Repeat
    a$=NextDevice
  Until a$=n$ OR a$=""
  t=DeviceEntryType
  FreeDeviceList
EndIf
r=AssignLock_(&n$,0)
NPrint "while locked ",t
u=DeviceEntryType
If u=t Then NPrint "after FreeDeviceList: the same" Else NPrint "after FreeDeviceList: other"
End
