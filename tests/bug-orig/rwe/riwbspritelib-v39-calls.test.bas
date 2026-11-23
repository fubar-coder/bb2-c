; riwbspritelib-v39-calls (#1151) - the library opens graphics.library in
; any version but makes its sprites with V39 functions (AllocBitMap,
; AllocSpriteDataA, GetExtSpriteA, ChangeExtSpriteA, FreeSpriteData): on
; Kickstart 1.x and 2.x (graphics V33-V38) GetWBSprite passes its only
; guard (the base is set) and calls behind the end of graphics.library's
; jump table, the machine crashes (the session ends in a timeout).  The
; same program on Kickstart 1.3, 2.04 and 3.1: a sprite on
; channel 2 from a 16-pixel shape, its position, MoveWBSprite,
; ChangeWBSprite with a 20-pixel shape (a sprite is 16 pixels wide),
; GetWBSprite with a 32-pixel shape (refused: a sprite below V39 is 16
; pixels wide, and the Workbench screens here give no wider ones),
; FreeWBSprite, GetWBSprite on any channel.
; Correct: on every Kickstart: 2, "10 20", "-1 100 50", -1, -1, 1.
DEFTYPE .l

!TestCase{sprites}
  BitMap 0,320,64,2
  Boxf 0,0,15,15,3
  Boxf 0,16,40,30,2
  GetaShape 0,0,0,16,16
  GetaShape 1,0,16,20,8
  GetaShape 2,0,0,32,8
  FindScreen 0
  vp=ViewPort(0)
  s=GetWBSprite(10,20,0,2)
  x1=WBSpriteX(2)
  y1=WBSpriteY(2)
  r.w=MoveWBSprite(vp,100,50,2)
  x2=WBSpriteX(2)
  y2=WBSpriteY(2)
  c=ChangeWBSprite(vp,2,1)
  t=GetWBSprite(10,20,2,3)
  FreeWBSprite 2
  u=GetWBSprite(1,2,0,-1)
  !CheckEq{s,2,"channel"}
  !CheckEq{x1,10,"x"}
  !CheckEq{y1,20,"y"}
  !CheckEq{r,-1,"moved"}
  !CheckEq{x2,100,"moved_x"}
  !CheckEq{y2,50,"moved_y"}
  !CheckEq{c,-1,"changed"}
  !CheckEq{t,-1,"wide_refused"}
  !CheckEq{u,1,"any_channel"}
!EndTestCase
