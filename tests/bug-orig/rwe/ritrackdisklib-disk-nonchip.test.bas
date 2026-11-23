; ritrackdisklib-disk-nonchip (#1163) - the sector commands pass the
; program's buffer to trackdisk.device as it is; before Kickstart 2.0 the
; device moves the data by DMA and needs the buffer in chip memory.  With a
; buffer outside chip memory the command returns 0, but the device drops
; the address's upper bits: a write takes its data from chip memory at that
; address, a read writes into it (measured on the a500: a buffer at
; $C19D90 wrote the data at $019D90 to the disk and a read overwrote them).
; A blank disk in DF1 (DF0 empty; "SECTOR 0000" at sector 0).  The buffers
; fb, fr, ft and bb are outside chip memory (MEMF_FAST: the slow memory of
; the a500 and a500-204, fast memory on the a1200), cb is chip memory.  fb
; holds the letters A-M at the start of its 13 sectors, ft a-k in its 11.
; WriteSector of 13 sectors from fb at sector 20 (across tracks 1 and 2),
; read back into cb and into fr; FormatTrack of track 4 from ft, read
; back; ReadSector of sector 0 into fr; BootProg from bb, read back.
; Before every read back the motor is off and trackdisk.device's track
; buffer is dropped (CMD_CLEAR, as a disk change would).
; Correct (on every Kickstart): the data are written and read: sectors 20,
; 21, 22, 32 = A B C M in both buffers, 44 = a, 54 = k, sector 0 "SECTOR
; 0000", then "DOS" after BootProg.
; The original crashes on Kickstart 1.3 at start-up (#941), before the
; first command, and the bug shows only with that fixed.
DEFTYPE .l

!TestCase{non_chip_buffers}
  fb=AllocMem_(6656,$10004)
  fr=AllocMem_(6656,$10004)
  ft=AllocMem_(5632,$10004)
  bb=AllocMem_(1024,$10004)
  cb=AllocMem_(6656,$10002)
  !Require{fb<>0 AND fr<>0 AND ft<>0 AND bb<>0 AND cb<>0,"memory"}
  !Require{(TypeOfMem_(fb) AND 2)=0,"fb_not_chip"}
  !Require{(TypeOfMem_(fr) AND 2)=0,"fr_not_chip"}
  !Require{(TypeOfMem_(ft) AND 2)=0,"ft_not_chip"}
  !Require{(TypeOfMem_(bb) AND 2)=0,"bb_not_chip"}
  For i=0 To 12 : Poke.b fb+i*512,65+i : Next
  For i=0 To 10 : Poke.b ft+i*512,97+i : Next
  r=OpenDisk(1)
  w1=WriteSector(1,20,fb,13) AND 255
  MotorOff 1
  req=DiskIO(1)
  Poke.w req+28,5
  r=DoIO_(req)
  r1=ReadSector(1,20,cb,13) AND 255
  c$=Peek$(cb)+" "+Peek$(cb+512)+" "+Peek$(cb+1024)+" "+Peek$(cb+6144)
  MotorOff 1
  req=DiskIO(1)
  Poke.w req+28,5
  r=DoIO_(req)
  r2=ReadSector(1,20,fr,13) AND 255
  f$=Peek$(fr)+" "+Peek$(fr+512)+" "+Peek$(fr+1024)+" "+Peek$(fr+6144)
  fm=FormatTrack(1,4,ft) AND 255
  MotorOff 1
  req=DiskIO(1)
  Poke.w req+28,5
  r=DoIO_(req)
  r3=ReadSector(1,44,cb,11) AND 255
  t$=Peek$(cb)+" "+Peek$(cb+5120)
  r4=ReadSector(1,0,fr) AND 255
  z$=Peek$(fr)
  bp=BootProg(1,bb) AND 255
  MotorOff 1
  req=DiskIO(1)
  Poke.w req+28,5
  r=DoIO_(req)
  r5=ReadSector(1,0,cb) AND 255
  b$=Peek$(cb)
  MotorOff 1
  CloseDisk 1
  FreeMem_ fb,6656
  FreeMem_ fr,6656
  FreeMem_ ft,5632
  FreeMem_ bb,1024
  FreeMem_ cb,6656
  !CheckEq{w1,0,"writesector"}
  !CheckEq{r1,0,"read_chip"}
  !CheckStrEq{c$,"A B C M","chip_data"}
  !CheckEq{r2,0,"read_fast"}
  !CheckStrEq{f$,"A B C M","fast_data"}
  !CheckEq{fm,0,"formattrack"}
  !CheckEq{r3,0,"read_format"}
  !CheckStrEq{t$,"a k","format_data"}
  !CheckEq{r4,0,"read_sector_0"}
  !CheckStrEq{z$,"SECTOR 0000","sector_0"}
  !CheckEq{bp,0,"bootprog"}
  !CheckEq{r5,0,"read_boot"}
  !CheckStrEq{b$,"DOS","boot_id"}
!EndTestCase
