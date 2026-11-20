; ritrackdisklib-disk-formattrack (#858) - FormatTrack and WriteBoot send
; TD_FORMAT with the sector commands' length, number x 512 bytes; the
; trackdisk.device formats whole tracks only (5632 bytes) and refuses any
; other length with -4 (the low byte 252): FormatTrack without a count
; and WriteBoot never write anything.
; A blank disk in DF1 (DF0 empty; marks "SECTOR 0011" at sector 11 and
; "SECTOR 0000" at sector 0); the buffer holds the letters A-K at the
; start of its 11 sectors (A-V for two tracks).  After the motor is off
; and trackdisk.device's track buffer is dropped (CMD_CLEAR) the sectors
; are read back from the disk.
; Correct: FormatTrack without a count formats one track (sector 11 = A,
; sector 21 = K); the count stays in sectors: 11 formats one track
; (sector 22 = A), 22 two (sectors 33 = A, 54 = V); WriteBoot 1,bootdata
; writes sectors 0-1 (0).
; @rwe ritrackdisklib
; @fixed #858
; @machine a1200 a500-204 a500
; @pass floppy=1|@TESTDATA@/ritrackdisklib-blank.adf
; @pass timeout=120
DEFTYPE .l
buf=AllocMem_(11264,$10002)
rb=AllocMem_(512,$10002)
For i=0 To 21 : Poke.b buf+i*512,65+i : Next
r=OpenDisk(1)
r=FormatTrack(1,1,buf)
NPrint "FormatTrack 1,1,buf: ",r&255
r=FormatTrack(1,2,buf,11)
NPrint "FormatTrack 1,2,buf,11: ",r&255
r=FormatTrack(1,3,buf,22)
NPrint "FormatTrack 1,3,buf,22: ",r&255
r=WriteBoot(1,buf)
NPrint "WriteBoot 1,buf: ",r&255
MotorOff 1
req=DiskIO(1)
Poke.w req+28,5
r=DoIO_(req)
s=0 : Gosub rd
s=11 : Gosub rd
s=21 : Gosub rd
s=22 : Gosub rd
s=32 : Gosub rd
s=33 : Gosub rd
s=54 : Gosub rd
CloseDisk 1
FreeMem_ buf,11264
FreeMem_ rb,512
End
rd:
r=ReadSector(1,s,rb)
NPrint "sector ",s,": ",r&255," ",Peek$(rb)
Return
