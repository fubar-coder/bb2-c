; ciatrklib-decode-chip-signed (#432) - DecodeModule leaves a module where
; it is if its address is below $200000 (taken as chip memory) and copies
; it to chip memory otherwise, but compares signed (CMP.l #$00200000,d0:
; BLT): every address from $80000000 up counts as chip memory.  The
; emulator runs with 24 bit addressing (as a 68000 or 68EC020 Amiga), so
; the address of a module in chip memory plus $80000000 reaches the same
; bytes; DecodeModule gets that address (a module at $80000000 or above,
; as in the memory of some accelerator boards).
; Correct: the module is copied to chip memory: GetTrackerLocation is
; below $200000 and the copy holds the module's bytes.
DEFTYPE .l

!TestCase{decode_high}
  MOVE.l #2200,d0 : MOVE.l #$10002,d1 : MOVE.l 4,a6 : JSR -198(a6) : PutReg d0,m
  !Require{m<>0,"allocated"}
  Poke.l m,$12345678
  Poke.b m+950,1
  Poke.l m+$438,$4D2E4B2E
  h=m+$80000000
  DecodeModule 0,h
  l=GetTrackerLocation(0)
  !Check{l<>h,"copied"}
  !Check{(l AND $FFE00000)=0,"chip"}
  !Check{Peek.l(l AND $FFFFFF)=$12345678,"bytes"}
  !CheckEq{GetTrackerSize(0),2111,"size"}
  FreeTrackerModule 0
  GetReg d0,m : MOVE.l d0,a1 : MOVE.l #2200,d0 : MOVE.l 4,a6 : JSR -210(a6)
!EndTestCase
