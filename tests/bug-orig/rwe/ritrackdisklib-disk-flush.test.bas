; ritrackdisklib-disk-flush (#1154) - the library never sends CMD_UPDATE:
; trackdisk.device keeps written sectors in its track buffer until it
; changes the track, so after MotorOff and CloseDisk the last track
; written is not on the disk.
; A blank disk in DF1 (DF0 empty; "SECTOR 0011" at sector 11).  The
; program writes "WRITTEN" to sector 11, switches the motor off, drops
; trackdisk.device's track buffer (CMD_CLEAR, as a disk change would)
; and reads sector 11 again; then closes the unit, opens it again, drops
; the buffer and reads it once more.
; Correct: both reads give "WRITTEN" (MotorOff wrote the track first).
DEFTYPE .l

!TestCase{flush}
  wb=AllocMem_(512,$10002)
  rb=AllocMem_(512,$10002)
  !Require{wb<>0 AND rb<>0,"memory"}
  a$="WRITTEN" : For i=1 To Len(a$) : Poke.b wb+i-1,Asc(Mid$(a$,i,1)) : Next
  r=OpenDisk(1)
  wr=WriteSector(1,11,wb) AND 255
  MotorOff 1
  req=DiskIO(1)
  Poke.w req+28,5
  r=DoIO_(req)
  r1=ReadSector(1,11,rb) AND 255
  m$=Peek$(rb)
  r=WriteSector(1,12,wb)
  CloseDisk 1
  r=OpenDisk(1)
  req=DiskIO(1)
  Poke.w req+28,5
  r=DoIO_(req)
  r2=ReadSector(1,12,rb) AND 255
  c$=Peek$(rb)
  MotorOff 1
  CloseDisk 1
  FreeMem_ wb,512
  FreeMem_ rb,512
  !CheckEq{wr,0,"write_sector"}
  !CheckEq{r1,0,"read_after_motoroff"}
  !CheckStrEq{m$,"WRITTEN","after_motoroff"}
  !CheckEq{r2,0,"read_after_closedisk"}
  !CheckStrEq{c$,"WRITTEN","after_closedisk"}
!EndTestCase
