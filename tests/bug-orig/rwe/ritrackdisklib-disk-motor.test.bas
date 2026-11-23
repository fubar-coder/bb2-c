; ritrackdisklib-disk-motor (#857) - DiskMotor ("Returns -1 if motor is
; on") returns SNE of bit 7 of CIAB PRB, which trackdisk.device leaves 1
; after every request: -1 whatever the motor does.
; A blank disk in DF1 (DF0 empty).
; Correct: DiskMotor is the motor state the library knows: 0 before
; anything, -1 after MotorOn, 0 after MotorOff, -1 after a ReadSector
; (trackdisk.device switches the motor on), 0 after CloseDisk.
DEFTYPE .l

!TestCase{motor_state}
  rb=AllocMem_(512,$10002)
  !Require{rb<>0,"memory"}
  r=OpenDisk(1)
  m0=DiskMotor
  r=MotorOn(1)
  m1=DiskMotor
  r=MotorOff(1)
  m2=DiskMotor
  r=ReadSector(1,0,rb)
  m3=DiskMotor
  CloseDisk 1
  m4=DiskMotor
  FreeMem_ rb,512
  !CheckEq{m0,0,"opened"}
  !CheckEq{m1,-1,"after_motoron"}
  !CheckEq{m2,0,"after_motoroff"}
  !CheckEq{m3,-1,"after_readsector"}
  !CheckEq{m4,0,"after_closedisk"}
!EndTestCase
