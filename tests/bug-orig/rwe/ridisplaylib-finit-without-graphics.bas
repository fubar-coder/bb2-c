; ridisplaylib-finit-without-graphics (#891) - init opens
; graphics.library 39; finit returns at once when that failed, before it
; frees anything: on a Kickstart without graphics.library 39 even the
; last copper list of InitView is never freed (exec's AllocMem, chip
; memory).  The program runs through inject: openlibfail makes every
; OpenLibrary of graphics.library fail, track reports what the process
; leaves allocated.  The program makes one list and ends.
; Correct: nothing is left allocated.
; @rwe ridisplaylib
; @pass runwrap=@INJECT@|openlibfail=graphics.library track
DEFTYPE .l
c=InitView(256,5,8,32)
If c=0 Then NPrint "no list" Else NPrint "list made"
End
