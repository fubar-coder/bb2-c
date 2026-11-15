; shapetrixlib-rotate-plane-64k (#1047) - a shape keeps its plane sizes
; in words (+22 bytes per plane, +24 with the extra word of every row);
; Rotate makes a new shape of the rotated size without checking it: a
; plane of 64K or more gets wrong word sizes.
; Here with runtime error checking: a shape of 600 x 600 pixels (a plane
; of 46800 bytes with the extra word), Rotate 0,0.125 (45 degrees: about
; 849 x 849 pixels, 108 bytes per row, 93390 bytes with the extra word).
; Correct: an error check refuses the new shape: "Shape too Big", the
; command is not done.
; @fixed #1047
; @configs clib
; @pass runtime_errors=on
; @pass timeout=300
DEFTYPE .l
InitShape 0,600,600,1
NPrint "before"
Rotate 0,0.125
NPrint "after, ",ShapeWidth(0)," x ",ShapeHeight(0)
End
