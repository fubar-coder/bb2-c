; riwbspritelib-v39-shape-too-big-kick204 (#1151) -
; riwbspritelib-v39-shape-too-big on Kickstart 2.04 (graphics V37): the
; original's V39 call at the first GetWBSprite ends in another exception
; than on 1.3 (Blitz's debugger: "Line 1111 Emulator").
; Correct: the error check refuses the 32-pixel shape: "Shape too big for
; a sprite below Kickstart 3.0" after "before".
; @rwe riwbspritelib
; @machine a500-204
; @pass runtime_errors=on
; @pass timeout=30
BitMap 0,320,64,2
Boxf 0,0,31,7,3
GetaShape 0,0,0,32,8
NPrint "before"
s.l=GetWBSprite(10,20,0,2)
NPrint s
End
