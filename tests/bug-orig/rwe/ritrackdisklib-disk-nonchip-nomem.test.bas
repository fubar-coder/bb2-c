; ritrackdisklib-disk-nonchip-nomem (#1163) - on Kickstart 1.x the sector
; commands with a buffer outside chip memory: trackdisk.device needs chip
; memory and drops the address's upper bits (ritrackdisklib-disk-nonchip).
; The fix sends such a buffer through a chip buffer of the library; this
; test checks the case without chip memory for it.
; A blank disk in DF1 (DF0 empty; "SECTOR 0000" at sector 0).  fb is
; outside chip memory (the a500's slow memory).  The program takes all
; chip memory (AllocMem of the largest block until none is left), calls
; ReadSector of sector 0 into fb and WriteSector of sector 0 from fb, frees
; the chip memory again and reads sector 0 into fb once more.
; Correct: without chip memory both commands send nothing and return
; TDERR_NoMem (31), fb and the disk stay unchanged; afterwards ReadSector
; reads "SECTOR 0000".
; The original crashes on Kickstart 1.3 at start-up (#941), before the
; first command.
DEFTYPE .l

!TestCase{no_chip_memory}
  Dim blk.l(200),siz.l(200)
  fb=AllocMem_(512,$10004)
  !Require{fb<>0,"memory"}
  Poke.b fb,70 : Poke.b fb+1,66
  r=OpenDisk(1)
  n=0
  Repeat
    s=AvailMem_($20002)
    If s>0
      blk(n)=AllocMem_(s,2)
      If blk(n)=0 Then s=0 Else siz(n)=s : n+1
    EndIf
  Until s=0 OR n=200
  c=AvailMem_(2)
  r1=ReadSector(1,0,fb) AND 255
  fbs$=Peek$(fb)
  r2=WriteSector(1,0,fb) AND 255
  For i=0 To n-1 : FreeMem_ blk(i),siz(i) : Next
  MotorOff 1
  req=DiskIO(1)
  Poke.w req+28,5
  r=DoIO_(req)
  r3=ReadSector(1,0,fb) AND 255
  after$=Peek$(fb)
  MotorOff 1
  CloseDisk 1
  FreeMem_ fb,512
  !CheckEq{c,0,"chip_left"}
  !CheckEq{r1,31,"read_no_memory"}
  !CheckStrEq{fbs$,"FB","buffer_unchanged"}
  !CheckEq{r2,31,"write_no_memory"}
  !CheckEq{r3,0,"read_after"}
  !CheckStrEq{after$,"SECTOR 0000","disk_unchanged"}
!EndTestCase
