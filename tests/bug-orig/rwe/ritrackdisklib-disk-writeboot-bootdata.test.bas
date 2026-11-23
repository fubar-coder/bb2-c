; ritrackdisklib-disk-writeboot-bootdata (#864) - WriteBoot unit,bootdata
; loads the sector 0 into d1 over its argument bootdata and writes from
; d2, which is no argument (whatever the program left there).
; A blank disk in DF1 (DF0 empty; "SECTOR 0000" at sector 0); bootdata
; holds "BOOTDATA 0" and "BOOTDATA 1" at the start of its two sectors.
; After the motor is off and trackdisk.device's track buffer is dropped
; (CMD_CLEAR) sectors 0-1 are read back from the disk.
; Correct: sectors 0 and 1 hold bootdata.
DEFTYPE .l

!TestCase{bootdata}
  bd=AllocMem_(1024,$10002)
  rb=AllocMem_(1024,$10002)
  !Require{bd<>0 AND rb<>0,"memory"}
  a$="BOOTDATA 0" : For i=1 To Len(a$) : Poke.b bd+i-1,Asc(Mid$(a$,i,1)) : Next
  a$="BOOTDATA 1" : For i=1 To Len(a$) : Poke.b bd+511+i,Asc(Mid$(a$,i,1)) : Next
  r=OpenDisk(1)
  wbt=WriteBoot(1,bd) AND 255
  MotorOff 1
  req=DiskIO(1)
  Poke.w req+28,5
  r=DoIO_(req)
  rd=ReadSector(1,0,rb,2) AND 255
  s0$=Peek$(rb)
  s1$=Peek$(rb+512)
  CloseDisk 1
  FreeMem_ bd,1024
  FreeMem_ rb,1024
  !CheckEq{wbt,0,"writeboot"}
  !CheckEq{rd,0,"read_back"}
  !CheckStrEq{s0$,"BOOTDATA 0","sector_0"}
  !CheckStrEq{s1$,"BOOTDATA 1","sector_1"}
!EndTestCase
