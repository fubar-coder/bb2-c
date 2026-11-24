; rigfxlib-cpucls-no-plane-crash (#680) - CPUCls tests the first plane
; pointer before it saves a4-a6 (MOVEM.l a4-a6,-(a7)); without a plane it
; branches to the end of the routine, which restores a4-a6 from the
; stack and returns: it pops the return address and two longs of the
; caller into a4-a6 and returns to the next long.  Bitmap 5 was never
; set up: its plane 0 is 0.
; To give that wrong return a defined end the program pushes the address
; of rec, a6 and a5 before the call: the original pops the return address
; into a4 (the base register of the driver's frame, saved in sv and
; put back at rec), a5 and a6 their own values, and returns to rec, with
; the stack as before the pushes; the program then sets w=1.  The
; correct routine returns after the call and the program drops the three
; longs.  This relies on details outside the bug: that the call leaves
; a4-a6 as they were and that the driver's Gosub frame is above the
; pushed longs.
; Correct: CPUCls returns without doing anything, the program goes on
; (w stays 0).
DEFTYPE .l

!TestCase{no_plane}
  w=0
  LEA sv(pc),a0 : MOVE.l a4,(a0)
  LEA rec(pc),a0 : MOVE.l a0,-(a7) : MOVE.l a6,-(a7) : MOVE.l a5,-(a7)
  CPUCls 5
  LEA 12(a7),a7
  Goto ok
rec:
  LEA sv(pc),a0 : MOVE.l (a0),a4
  w=1
ok:
  !CheckEq{w,0,"return_address"}
  !Check{1=1,"after"}
  Goto fin
sv:
  Dc.l 0
fin:
!EndTestCase
