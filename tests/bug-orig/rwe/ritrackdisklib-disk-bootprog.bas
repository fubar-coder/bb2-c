; ritrackdisklib-disk-bootprog (#856) - BootProg stores as checksum the
; plain sum with carry of the 256 longs, including the old checksum long,
; not its complement, and clears the root block long after the sum:
; Kickstart does not boot the block.
; A blank disk in DF1 (DF0 empty); the block in memory is 1024 bytes of
; "BOOT" with $12345678 as old checksum.  After the motor is off and
; trackdisk.device's track buffer is dropped (CMD_CLEAR) sectors 0-1 are
; read back and added as Strap does: the 256 longs with end-around carry.
; Correct: "DOS", root block 0, the sum of the block read back is
; $FFFFFFFF (-1).
; @rwe ritrackdisklib
; @fixed #856
; @machine a1200 a500-204 a500
; @pass floppy=1|@TESTDATA@/ritrackdisklib-blank.adf
; @pass timeout=120
DEFTYPE .l
mem=AllocMem_(1024,$10002)
rb=AllocMem_(1040,$10002)
For i=0 To 255 : Poke.l mem+i*4,$424F4F54 : Next
Poke.l mem+4,$12345678
r=OpenDisk(1)
r=BootProg(1,mem)
NPrint "BootProg: ",r&255
MotorOff 1
req=DiskIO(1)
Poke.w req+28,5
r=DoIO_(req)
r=ReadSector(1,0,rb,2)
NPrint "read back: ",r&255
NPrint "id: ",Left$(Peek$(rb),3)," root block: ",Peek.l(rb+8)
GetReg d0,rb
MOVEM.l d2-d7/a2-a6,-(a7)
MOVE.l d0,a3
MOVEQ #0,d0
MOVE.l a3,a0
MOVE.w #255,d1
sumloop:
ADD.l (a0)+,d0
BCC sumnc
ADDQ.l #1,d0
sumnc:
DBRA d1,sumloop
MOVE.l d0,1024(a3)
MOVEM.l (a7)+,d2-d7/a2-a6
NPrint "sum: ",Peek.l(rb+1024)
CloseDisk 1
FreeMem_ mem,1024
FreeMem_ rb,1040
End
