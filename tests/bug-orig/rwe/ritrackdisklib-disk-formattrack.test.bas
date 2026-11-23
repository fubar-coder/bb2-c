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
DEFTYPE .l

!TestCase{format_tracks}
  buf=AllocMem_(11264,$10002)
  rb=AllocMem_(512,$10002)
  !Require{buf<>0 AND rb<>0,"memory"}
  For i=0 To 21 : Poke.b buf+i*512,65+i : Next
  r=OpenDisk(1)
  f1=FormatTrack(1,1,buf) AND 255
  f2=FormatTrack(1,2,buf,11) AND 255
  f3=FormatTrack(1,3,buf,22) AND 255
  wbt=WriteBoot(1,buf) AND 255
  MotorOff 1
  req=DiskIO(1)
  Poke.w req+28,5
  r=DoIO_(req)
  rs=0
  rs=rs+(ReadSector(1,0,rb) AND 255) : s0$=Peek$(rb)
  rs=rs+(ReadSector(1,11,rb) AND 255) : s11$=Peek$(rb)
  rs=rs+(ReadSector(1,21,rb) AND 255) : s21$=Peek$(rb)
  rs=rs+(ReadSector(1,22,rb) AND 255) : s22$=Peek$(rb)
  rs=rs+(ReadSector(1,32,rb) AND 255) : s32$=Peek$(rb)
  rs=rs+(ReadSector(1,33,rb) AND 255) : s33$=Peek$(rb)
  rs=rs+(ReadSector(1,54,rb) AND 255) : s54$=Peek$(rb)
  CloseDisk 1
  FreeMem_ buf,11264
  FreeMem_ rb,512
  !CheckEq{f1,0,"formattrack_no_count"}
  !CheckEq{f2,0,"formattrack_11"}
  !CheckEq{f3,0,"formattrack_22"}
  !CheckEq{wbt,0,"writeboot"}
  !CheckEq{rs,0,"reads"}
  !CheckStrEq{s0$,"A","sector_0"}
  !CheckStrEq{s11$,"A","sector_11"}
  !CheckStrEq{s21$,"K","sector_21"}
  !CheckStrEq{s22$,"A","sector_22"}
  !CheckStrEq{s32$,"K","sector_32"}
  !CheckStrEq{s33$,"A","sector_33"}
  !CheckStrEq{s54$,"V","sector_54"}
!EndTestCase
