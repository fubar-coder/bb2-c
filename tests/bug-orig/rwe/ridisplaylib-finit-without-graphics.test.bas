; ridisplaylib-finit-without-graphics (#891) - init opens
; graphics.library 39; finit returns at once when that failed, before it
; frees anything: on a Kickstart without graphics.library 39 even the
; last copper list of InitView is never freed (exec's AllocMem, chip
; memory).  The program runs through inject: openlibfail makes every
; OpenLibrary of graphics.library fail, track reports what the process
; leaves allocated.  The program makes one list and ends.  The height
; equals the colour count, so the list is 502 bytes with the height or
; with the colours in its size (#890).
; Correct: nothing is left allocated.
DEFTYPE .l

!TestCase{one_list}
  c=InitView(32,5,8,32)
  !Check{c<>0,"list_made"}
!EndTestCase
