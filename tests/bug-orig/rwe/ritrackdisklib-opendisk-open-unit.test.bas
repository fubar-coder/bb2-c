; ritrackdisklib-opendisk-open-unit (#862) - OpenDisk of a unit that is
; open makes a new request and opens the device again: it returns -1, and
; the first request stays open and is lost (CloseDisk closes only the new
; one).
; Correct: an open unit is left alone, the second OpenDisk returns 0; one
; CloseDisk closes the unit.
DEFTYPE .l

!TestCase{open_twice}
  o1=OpenDisk(0)
  o2=OpenDisk(0)
  c1=CloseDisk(0)
  c2=CloseDisk(0)
  !CheckEq{o1,-1,"first_open"}
  !CheckEq{o2,0,"second_open"}
  !CheckEq{c1,-1,"first_close"}
  !CheckEq{c2,0,"second_close"}
!EndTestCase
