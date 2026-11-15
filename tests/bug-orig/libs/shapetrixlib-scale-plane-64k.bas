; shapetrixlib-scale-plane-64k (#1047) - a shape keeps its plane sizes
; in words (+22 bytes per plane, +24 with the extra word of every row);
; Scale makes a new shape of the scaled size without checking it: a
; plane of 64K or more gets wrong word sizes.
; Here with runtime error checking: a shape of 256 x 500 pixels, Scale
; 0,2,2 (512 x 1000: 64 bytes per row, a plane of 64000 bytes, 66000
; with the extra word).
; Correct: an error check refuses the new shape: "Shape too Big", the
; command is not done.
; @fixed #1047
; @configs clib
; @pass runtime_errors=on
; @pass timeout=300
DEFTYPE .l
InitShape 0,256,500,1
NPrint "before"
Scale 0,2,2
NPrint "after, ",ShapeWidth(0)," x ",ShapeHeight(0)
End
