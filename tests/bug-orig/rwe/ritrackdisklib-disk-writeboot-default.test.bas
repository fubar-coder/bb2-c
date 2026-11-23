; ritrackdisklib-disk-writeboot-default (#865) - WriteBoot unit writes the
; library's 8-byte default block ("DOS",0 and the long -1) and the 1016
; bytes behind it in memory (and, as TD_FORMAT of 1024 bytes, is refused
; by trackdisk.device: nothing is written, #858).
; A blank disk in DF1 (DF0 empty); the program writes the boot block,
; switches the motor off, drops trackdisk.device's track buffer
; (CMD_CLEAR) and reads sectors 0-1 back from the disk.  It checks the
; block as Strap does: "DOS", the sum of the 256 longs with end-around
; carry ($FFFFFFFF = -1), and runs the boot code (a6 = SysBase): d0 0 and
; a0 the dos.library resident's rt_Init.
; Correct: a bootable standard boot block on every Kickstart: "DOS",0
; (the blank disk has no DOS block), root block 880, sum -1, the boot code
; finds dos.library.
DEFTYPE .l

!TestCase{default_block}
  buf=AllocMem_(1040,$10002)
  !Require{buf<>0,"memory"}
  r=OpenDisk(1)
  wbt=WriteBoot(1) AND 255
  MotorOff 1
  req=DiskIO(1)
  Poke.w req+28,5
  r=DoIO_(req)
  rd=ReadSector(1,0,buf,2) AND 255
  id$=Left$(Peek$(buf),3)
  idb=Peek.b(buf+3) AND 255
  root=Peek.l(buf+8)
  GetReg d0,buf
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
  sum=Peek.l(buf+1024)
  bootable=(Peek$(buf)="DOS" AND sum=-1)
  If bootable
    GetReg d0,buf
    MOVEM.l d2-d7/a2-a6,-(a7)
    MOVE.l d0,a3
    LEA 12(a3),a2
    SUB.l a1,a1
    MOVE.l 4,a6
    JSR (a2)
    MOVE.l d0,1028(a3)
    MOVE.l a0,1032(a3)
    MOVEM.l (a7)+,d2-d7/a2-a6
    rt=FindResident_("dos.library")
    bd0=Peek.l(buf+1028)
    ba0=(Peek.l(buf+1032)=Peek.l(rt+22))
    !CheckEq{bd0,0,"boot_code_d0"}
    !CheckEq{ba0,-1,"boot_code_a0"}
  EndIf
  CloseDisk 1
  FreeMem_ buf,1040
  !CheckEq{wbt,0,"writeboot"}
  !CheckEq{rd,0,"read_back"}
  !CheckStrEq{id$,"DOS","id"}
  !CheckEq{idb,0,"id_byte"}
  !CheckEq{root,880,"root_block"}
  !CheckEq{sum,-1,"sum"}
  !CheckEq{bootable,-1,"bootable"}
!EndTestCase
