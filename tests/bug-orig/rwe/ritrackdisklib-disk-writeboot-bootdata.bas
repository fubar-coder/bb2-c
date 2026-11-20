; ritrackdisklib-disk-writeboot-bootdata (#864) - WriteBoot unit,bootdata
; loads the sector 0 into d1 over its argument bootdata and writes from
; d2, which is no argument (whatever the program left there).
; A blank disk in DF1 (DF0 empty; "SECTOR 0000" at sector 0); bootdata
; holds "BOOTDATA 0" and "BOOTDATA 1" at the start of its two sectors.
; After the motor is off and trackdisk.device's track buffer is dropped
; (CMD_CLEAR) sectors 0-1 are read back from the disk.
; Correct: sectors 0 and 1 hold bootdata.
; @rwe ritrackdisklib
; @fixed #864
; @machine a1200 a500-204 a500
; @pass floppy=1|@TESTDATA@/ritrackdisklib-blank.adf
; @pass timeout=120
DEFTYPE .l
bd=AllocMem_(1024,$10002)
rb=AllocMem_(1024,$10002)
a$="BOOTDATA 0" : For i=1 To Len(a$) : Poke.b bd+i-1,Asc(Mid$(a$,i,1)) : Next
a$="BOOTDATA 1" : For i=1 To Len(a$) : Poke.b bd+511+i,Asc(Mid$(a$,i,1)) : Next
r=OpenDisk(1)
r=WriteBoot(1,bd)
NPrint "WriteBoot 1,bootdata: ",r&255
MotorOff 1
req=DiskIO(1)
Poke.w req+28,5
r=DoIO_(req)
r=ReadSector(1,0,rb,2)
NPrint "read back: ",r&255
NPrint "sector 0: ",Peek$(rb)
NPrint "sector 1: ",Peek$(rb+512)
CloseDisk 1
FreeMem_ bd,1024
FreeMem_ rb,1024
End
