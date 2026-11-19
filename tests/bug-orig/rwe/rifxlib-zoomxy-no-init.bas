; rifxlib-zoomxy-no-init (#1131) - ZoomXY works on the bitmaps InitZoomXY
; stored and does not look whether there are any: before the first
; InitZoomXY both are 0, so ZoomXY reads a struct BitMap at address 0 -
; its depth a byte of the ExecBase pointer at address 4, its plane
; pointers the exception vectors - and writes through them.  ZoomXY is
; the first command of the program.  The original has no error check
; area.  Here with runtime error checking.
; Correct: the error check refuses ZoomXY without an InitZoomXY: "ZoomXY
; needs InitZoomXY", nothing is written.
; @fixed #1131
; @rwe rifxlib
; @pass timeout=30
; @pass runtime_errors=on
BitMap 0,320,16,1
NPrint "before"
ZoomXY 2,2,8
NPrint "after"
End
