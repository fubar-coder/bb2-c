; ritrackdisklib-opendisk-kept-name (#859) - OpenDisk unit,name$ copies
; the name over the library's "trackdisk.device", and OpenDisk unit
; without a name opens the device of that name: after a failed OpenDisk
; 0,"nosuch.device" OpenDisk 0 fails too.
; Correct: OpenDisk unit always opens trackdisk.device; unit 0 (DF0)
; opens also without a disk in the drive.
DEFTYPE .l

!TestCase{kept_name}
  r1=OpenDisk(0,"nosuch.device")
  r2=OpenDisk(0)
  r3=CloseDisk(0)
  !CheckEq{r1,0,"nosuch_fails"}
  !CheckEq{r2,-1,"opens_trackdisk"}
  !CheckEq{r3,-1,"closed"}
!EndTestCase
