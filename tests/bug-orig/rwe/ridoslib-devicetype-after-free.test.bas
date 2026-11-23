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
DEFTYPE .l

!TestCase{read_after_free}
  n$="RWEBUGORIG622"
  l=Lock_("RAM:",-2)
  !Require{l<>0,"locked"}
  r=AssignLock_(&n$,l)
  !Require{r<>0,"assign"}
  t=-2
  If GetDeviceList(16)
    Repeat
      a$=NextDevice
    Until a$=n$ OR a$=""
    t=DeviceEntryType
    FreeDeviceList
  EndIf
  r=AssignLock_(&n$,0)
  u=DeviceEntryType
  !CheckEq{t,1,"while_locked"}
  !CheckEq{u,t,"after_free_list"}
!EndTestCase
