; displaylib-init-sprite-pointer (#413) - the init of displaylib writes
; the address of its blank sprite into the template of the sprite moves
; at +2/+6 (the data of the SPR0POS/SPR0CTL moves) instead of +10/+14
; (SPR0PTH/SPR0PTL); InitCopList copies the template for every channel.
; Amiga mode (the list is built in memory only, as in t164_display): a
; CopList with 8 sprite channels; the moves of channel 0 (+12 of the
; CopList: 16 bytes per channel, POS, CTL, PTH, PTL).
; Correct: the sprite pointer of an unused channel is the blank sprite
; (not 0), its position and control words are 0.
DEFTYPE .l

!TestCase{channel_0}
  InitCopList 0,44,32,$10004,8,32,80
  p=Peek.l(Addr CopList(0)+12)
  !CheckEq{Peek.w(p),$140,"reg_pos"}
  !CheckEq{Peek.w(p+4),$142,"reg_ctl"}
  !CheckEq{Peek.w(p+8),$120,"reg_pth"}
  !CheckEq{Peek.w(p+12),$122,"reg_ptl"}
  v=((Peek.w(p+10) AND $FFFF) LSL 16) | (Peek.w(p+14) AND $FFFF)
  !Check{v<>0,"pointer_set"}
  !Check{(Peek.w(p+2)=0) AND (Peek.w(p+6)=0),"pos_ctl_zero"}
!EndTestCase
