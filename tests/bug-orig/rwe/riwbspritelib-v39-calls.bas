; riwbspritelib-v39-calls (#1151) - the library opens graphics.library in
; any version but makes its sprites with V39 functions (AllocBitMap,
; AllocSpriteDataA, GetExtSpriteA, ChangeExtSpriteA, FreeSpriteData): on
; Kickstart 1.x and 2.x (graphics V33-V38) GetWBSprite passes its only
; guard (the base is set) and calls behind the end of graphics.library's
; jump table, the machine crashes (the session ends in a timeout).  The
; same program on Kickstart 1.3 and 3.1 (on 2.04, whose original crashes
; with another requester: riwbspritelib-v39-calls-kick204): a sprite on
; channel 2 from a 16-pixel shape, its position, MoveWBSprite,
; ChangeWBSprite with a 20-pixel shape (a sprite is 16 pixels wide),
; GetWBSprite with a 32-pixel shape (refused: a sprite below V39 is 16
; pixels wide, and the Workbench screens here give no wider ones),
; FreeWBSprite, GetWBSprite on any channel.
; Correct: on every Kickstart: 2, "10 20", "-1 100 50", -1, -1, 1.
; @rwe riwbspritelib
; @machine a500 a1200
; @pass timeout=30
BitMap 0,320,64,2
Boxf 0,0,15,15,3
Boxf 0,16,40,30,2
GetaShape 0,0,0,16,16
GetaShape 1,0,16,20,8
GetaShape 2,0,0,32,8
FindScreen 0
vp.l=ViewPort(0)
NPrint "before"
s.l=GetWBSprite(10,20,0,2)
NPrint s
NPrint WBSpriteX(2)," ",WBSpriteY(2)
r.w=MoveWBSprite(vp,100,50,2)
NPrint r," ",WBSpriteX(2)," ",WBSpriteY(2)
r=ChangeWBSprite(vp,2,1)
NPrint r
t.l=GetWBSprite(10,20,2,3)
NPrint t
FreeWBSprite 2
u.l=GetWBSprite(1,2,0,-1)
NPrint u
End
