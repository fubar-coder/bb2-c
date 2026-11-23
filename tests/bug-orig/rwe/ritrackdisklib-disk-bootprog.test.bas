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
DEFTYPE .l

!TestCase{boot_block}
  mem=AllocMem_(1024,$10002)
  rb=AllocMem_(1040,$10002)
  !Require{mem<>0 AND rb<>0,"memory"}
  For i=0 To 255 : Poke.l mem+i*4,$424F4F54 : Next
  Poke.l mem+4,$12345678
  r=OpenDisk(1)
  boot=BootProg(1,mem) AND 255
  MotorOff 1
  req=DiskIO(1)
  Poke.w req+28,5
  r=DoIO_(req)
  rd=ReadSector(1,0,rb,2) AND 255
  id$=Left$(Peek$(rb),3)
  root=Peek.l(rb+8)
  GetReg d0,rb
  MOVEM.l d2-d7/a2-a6,-(a7)
  MOVE.l d0,a3
  MOVEQ #0,d0
  MOVE.l a3,a0
  MOVE.w #255,d1
  .sumloop
  ADD.l (a0)+,d0
  BCC sumnc
  ADDQ.l #1,d0
  .sumnc
  DBRA d1,sumloop
  MOVE.l d0,1024(a3)
  MOVEM.l (a7)+,d2-d7/a2-a6
  sum=Peek.l(rb+1024)
  CloseDisk 1
  FreeMem_ mem,1024
  FreeMem_ rb,1040
  !CheckEq{boot,0,"bootprog"}
  !CheckEq{rd,0,"read_back"}
  !CheckStrEq{id$,"DOS","id"}
  !CheckEq{root,0,"root_block"}
  !CheckEq{sum,-1,"sum"}
!EndTestCase
