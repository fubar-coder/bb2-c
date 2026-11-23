; ridoslib-nextdevice-no-list-length (#625) - NextDevice is a string
; function (result at a3, its length in d0).  Without a locked list (no
; GetDeviceList, or after FreeDeviceList) it returns at once without
; setting d0: the result's length is whatever d0 held on entry, the
; characters whatever the string buffer holds there.  Here d0 holds the
; 3 that the assignment before the call left in it.
; Correct: NextDevice without a locked list returns "" (length 0), before
; any GetDeviceList and after FreeDeviceList.
DEFTYPE .l

!TestCase{no_list}
  n=3 : a$=NextDevice
  la=Len(a$)
  r=GetDeviceList(8)
  !Require{r<>0,"list"}
  FreeDeviceList
  n=3 : b$=NextDevice
  lb=Len(b$)
  !CheckEq{la,0,"before_list"}
  !CheckEq{lb,0,"after_free"}
!EndTestCase
