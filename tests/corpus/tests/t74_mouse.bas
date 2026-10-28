; t74_mouse - mouselib in Blitz mode (without Mouse On: no interrupt):
; MouseArea, PositionMouse, MouseX/MouseY/MouseXSpeed/MouseYSpeed (Quick),
; Pointer without arguments
DEFTYPE .l
BLITZ
a.q=MouseX : b.q=MouseY : c.q=MouseXSpeed : d.q=MouseYSpeed
PositionMouse 30,40
e.q=MouseX : f.q=MouseY
MouseArea 0,0,100,50
PositionMouse 500,-3
g.q=MouseX : h.q=MouseY
Pointer
AMIGA
NPrint "init ",a," ",b," ",c," ",d
NPrint "pos ",e," ",f
NPrint "unclipped ",g," ",h
End
