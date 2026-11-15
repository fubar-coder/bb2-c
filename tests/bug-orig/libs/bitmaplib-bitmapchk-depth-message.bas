; bitmaplib-bitmapchk-depth-message (#1040) - the error check of BitMap
; (_bitmapchk) refuses a depth of 0 or more than 8 with error number 1,
; which the debugger shows as "No currently used object!": a misleading
; message for a wrong depth.
; Here with runtime error checking: BitMap with depth 2 (no error), then
; with depth 0.
; Correct: the runtime error names the depth: "Illegal BitMap Depth" (the
; text of CludgeBitMap's check).
; @fixed #1040
; @configs clib
; @pass runtime_errors=on
BitMap 0,64,10,2
NPrint "depth 2"
BitMap 1,64,10,0
NPrint "depth 0"
End
