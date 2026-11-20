; rilesdebuglib-fprint-bitmap-check (#707) - the font printer writes rows
; of 80 bytes (a bitmap of 640 pixels) and never looks at the used
; bitmap's size: into a narrower bitmap the characters land in the wrong
; rows, and rows below the bitmap are written as well.  A bitmap of 320
; pixels is an invalid argument (D-005); the original has no error check
; area.  Here with runtime error checking.
; Correct: the error check refuses the bitmap: "Font printer needs a
; bitmap of 640 pixels".
; @fixed #707
; @rwe rilesdebuglib
; @pass runtime_errors=on
DEFTYPE .l
BitMap 0,320,16,1
NPrint "before"
FLocate 0,0
FPrint "x"
NPrint "printed"
End
