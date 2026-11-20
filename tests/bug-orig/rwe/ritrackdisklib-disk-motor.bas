; ritrackdisklib-disk-motor (#857) - DiskMotor ("Returns -1 if motor is
; on") returns SNE of bit 7 of CIAB PRB, which trackdisk.device leaves 1
; after every request: -1 whatever the motor does.
; A blank disk in DF1 (DF0 empty).
; Correct: DiskMotor is the motor state the library knows: 0 before
; anything, -1 after MotorOn, 0 after MotorOff, -1 after a ReadSector
; (trackdisk.device switches the motor on), 0 after CloseDisk.
; @rwe ritrackdisklib
; @fixed #857
; @machine a1200 a500-204 a500
; @pass floppy=1|@TESTDATA@/ritrackdisklib-blank.adf
; @pass timeout=120
DEFTYPE .l
rb=AllocMem_(512,$10002)
r=OpenDisk(1)
NPrint "opened: ",DiskMotor
r=MotorOn(1)
NPrint "after MotorOn: ",DiskMotor
r=MotorOff(1)
NPrint "after MotorOff: ",DiskMotor
r=ReadSector(1,0,rb)
NPrint "after ReadSector: ",DiskMotor
CloseDisk 1
NPrint "after CloseDisk: ",DiskMotor
FreeMem_ rb,512
End
