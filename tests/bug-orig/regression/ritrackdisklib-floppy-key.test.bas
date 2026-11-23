; ritrackdisklib-floppy-key - the key floppy=N|SRC of the configuration of
; the BASIC test model (a test of the runner, nothing of the library is
; under test): a blank image in DF1, DF0 stays empty; the session boots
; cold with its own writable copy.
; Correct: unit 0 has no disk (ReadSector 29), unit 1 reads the marks of the
; image at sectors 0, 11 and 1759, sector 1760 is behind the disk
; (IOERR_BADLENGTH, -4).

DEFTYPE .l

!TestCase{floppy_in_df1}
  buf.l=AllocMem_(1024,$10002)
  r.l=OpenDisk(0)
  r=ReadSector(0,0,buf)
  !CheckEq{r&255,29,"df0_no_disk"}
  CloseDisk 0
  r=OpenDisk(1)
  r=ReadSector(1,0,buf)
  !CheckEq{r&255,0,"df1_sector_0_result"}
  !CheckStrEq{Peek$(buf),"SECTOR 0000","df1_sector_0"}
  r=ReadSector(1,11,buf)
  !CheckEq{r&255,0,"df1_sector_11_result"}
  !CheckStrEq{Peek$(buf),"SECTOR 0011","df1_sector_11"}
  r=ReadSector(1,1759,buf)
  !CheckEq{r&255,0,"df1_sector_1759_result"}
  !CheckStrEq{Peek$(buf),"SECTOR 1759","df1_sector_1759"}
  r=ReadSector(1,1760,buf)
  !CheckEq{r&255,252,"df1_sector_1760_behind_disk"}
  MotorOff 1
  CloseDisk 1
!EndTestCase
