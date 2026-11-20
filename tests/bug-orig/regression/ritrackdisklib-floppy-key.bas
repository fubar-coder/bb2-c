; ritrackdisklib-floppy-key - the pass key floppy=N|SRC of the corpus runner (a test of the
; runner, nothing of the library is under test): a blank image in DF1, DF0 stays empty; the
; session boots cold with its own writable copy.  Not on the a500: the library crashes there
; at start-up (#941).
; Correct: unit 0 has no disk (ReadSector 29), unit 1 reads the marks of the image at
; sectors 0, 11 and 1759, sector 1760 is behind the disk (IOERR_BADLENGTH, -4).
; @rwe ritrackdisklib
; @machine a500-204 a1200
; @pass floppy=1|@TESTDATA@/ritrackdisklib-blank.adf
; @pass timeout=120
buf.l=AllocMem_(1024,$10002)
r.l=OpenDisk(0)
r=ReadSector(0,0,buf)
NPrint "DF0 ",r&255
CloseDisk 0
r=OpenDisk(1)
r=ReadSector(1,0,buf)
NPrint "DF1 sector 0: ",r&255," ",Peek$(buf)
r=ReadSector(1,11,buf)
NPrint "DF1 sector 11: ",r&255," ",Peek$(buf)
r=ReadSector(1,1759,buf)
NPrint "DF1 sector 1759: ",r&255," ",Peek$(buf)
r=ReadSector(1,1760,buf)
NPrint "DF1 sector 1760: ",r&255
MotorOff 1
CloseDisk 1
End
