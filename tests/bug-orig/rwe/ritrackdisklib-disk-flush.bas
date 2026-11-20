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
; @rwe ritrackdisklib
; @fixed #1154
; @machine a1200 a500-204 a500
; @pass floppy=1|@TESTDATA@/ritrackdisklib-blank.adf
; @pass timeout=120
DEFTYPE .l
wb=AllocMem_(512,$10002)
rb=AllocMem_(512,$10002)
a$="WRITTEN" : For i=1 To Len(a$) : Poke.b wb+i-1,Asc(Mid$(a$,i,1)) : Next
r=OpenDisk(1)
r=WriteSector(1,11,wb)
NPrint "WriteSector: ",r&255
MotorOff 1
req=DiskIO(1)
Poke.w req+28,5
r=DoIO_(req)
r=ReadSector(1,11,rb)
NPrint "after MotorOff: ",r&255," ",Peek$(rb)
r=WriteSector(1,12,wb)
CloseDisk 1
r=OpenDisk(1)
req=DiskIO(1)
Poke.w req+28,5
r=DoIO_(req)
r=ReadSector(1,12,rb)
NPrint "after CloseDisk: ",r&255," ",Peek$(rb)
MotorOff 1
CloseDisk 1
FreeMem_ wb,512
FreeMem_ rb,512
End
