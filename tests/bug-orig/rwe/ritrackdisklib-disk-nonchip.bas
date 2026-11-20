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
; first command: its recording on the a500 shows that crash, and the bug
; shows only with that fixed (the port before this fix on the a500: 0 for
; every command, empty strings for every sector and buffer read back).
; @rwe ritrackdisklib
; @fixed #1163
; @machine a500 a500-204 a1200
; @pass floppy=1|@TESTDATA@/ritrackdisklib-blank.adf
; @pass timeout=120
DEFTYPE .l
fb=AllocMem_(6656,$10004)
fr=AllocMem_(6656,$10004)
ft=AllocMem_(5632,$10004)
bb=AllocMem_(1024,$10004)
cb=AllocMem_(6656,$10002)
NPrint "chip: ",TypeOfMem_(fb) AND 2," ",TypeOfMem_(fr) AND 2," ",TypeOfMem_(ft) AND 2," ",TypeOfMem_(bb) AND 2
For i=0 To 12 : Poke.b fb+i*512,65+i : Next
For i=0 To 10 : Poke.b ft+i*512,97+i : Next
r=OpenDisk(1)
r=WriteSector(1,20,fb,13)
NPrint "WriteSector 1,20,fb,13: ",r&255
Gosub dropbuf
r=ReadSector(1,20,cb,13)
NPrint "ReadSector 1,20,cb,13: ",r&255," ",Peek$(cb)," ",Peek$(cb+512)," ",Peek$(cb+1024)," ",Peek$(cb+6144)
Gosub dropbuf
r=ReadSector(1,20,fr,13)
NPrint "ReadSector 1,20,fr,13: ",r&255," ",Peek$(fr)," ",Peek$(fr+512)," ",Peek$(fr+1024)," ",Peek$(fr+6144)
r=FormatTrack(1,4,ft)
NPrint "FormatTrack 1,4,ft: ",r&255
Gosub dropbuf
r=ReadSector(1,44,cb,11)
NPrint "ReadSector 1,44,cb,11: ",r&255," ",Peek$(cb)," ",Peek$(cb+5120)
r=ReadSector(1,0,fr)
NPrint "ReadSector 1,0,fr: ",r&255," ",Peek$(fr)
r=BootProg(1,bb)
NPrint "BootProg 1,bb: ",r&255
Gosub dropbuf
r=ReadSector(1,0,cb)
NPrint "ReadSector 1,0,cb: ",r&255," ",Peek$(cb)
MotorOff 1
CloseDisk 1
FreeMem_ fb,6656
FreeMem_ fr,6656
FreeMem_ ft,5632
FreeMem_ bb,1024
FreeMem_ cb,6656
End
dropbuf:
MotorOff 1
req=DiskIO(1)
Poke.w req+28,5
r=DoIO_(req)
Return
