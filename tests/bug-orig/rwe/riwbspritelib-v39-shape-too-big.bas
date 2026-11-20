; riwbspritelib-v39-shape-too-big (#1151) - below graphics.library V39
; (Kickstart 1.x and 2.x) the library makes its sprites with V39
; functions and crashes at the first GetWBSprite (with runtime error
; checking Blitz's debugger reports it).  A sprite of the older
; graphics.library is 16 pixels wide with 2 planes; a wider shape is an
; error of the program on such a machine.  Here with runtime error
; checking, a 32-pixel shape, on Kickstart 1.3 (2.04:
; riwbspritelib-v39-shape-too-big-kick204).
; Correct: the error check refuses the shape: "Shape too big for a sprite
; below Kickstart 3.0" after "before".
; @fixed #1151
; @rwe riwbspritelib
; @machine a500
; @pass runtime_errors=on
; @pass timeout=30
BitMap 0,320,64,2
Boxf 0,0,31,7,3
GetaShape 0,0,0,32,8
NPrint "before"
s.l=GetWBSprite(10,20,0,2)
NPrint s
End
