; riwbspritelib-v39-calls-kick204 (#1151) - riwbspritelib-v39-calls on
; Kickstart 2.04 (graphics V37): the original calls V39 functions behind
; the end of graphics.library's jump table at the first GetWBSprite and
; crashes ("Software Failure", the session ends in a timeout).
; Correct: as on Kickstart 1.3 and 3.1: 2, "10 20", "-1 100 50", -1, -1,
; 1.
; @fixed #1151
; @rwe riwbspritelib
; @machine a500-204
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
