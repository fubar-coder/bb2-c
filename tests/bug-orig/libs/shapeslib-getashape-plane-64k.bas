; shapeslib-getashape-plane-64k (#1047) - a shape keeps its plane sizes
; in words (+22 bytes per plane, +24 with the extra word of every row);
; GetaShape checks only the area: a shape with a plane of 64K or more
; gets wrong word sizes.
; Here with runtime error checking: GetaShape of an area of 640 x 820
; pixels of a bitmap of that size (a plane of 65600 bytes).
; Correct: an error check refuses the shape: "Shape too Big", the
; command is not done.
; @fixed #1047
; @configs clib
; @pass runtime_errors=on
DEFTYPE .l
BitMap 0,640,820,1
NPrint "before"
GetaShape 0,0,0,640,820
NPrint "after, plane ",Peek.w(Addr Shape(0)+22) AND $FFFF
End
