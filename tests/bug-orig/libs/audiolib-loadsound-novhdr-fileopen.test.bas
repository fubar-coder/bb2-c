; audiolib-loadsound-novhdr-fileopen (#67) - LoadSound and DiskPlay open
; the file (doopensound, iffiolib iffopen) and go to the runtime errors
; "Can't Find VHDR" and "Error Reading File" without iffclose: the file
; stays open (iffiolib has no finit, also after the program ends).  The
; 8SVX here (generated) has no VHDR chunk.
; Here: SetErr catches the error; the handler asks for an exclusive lock
; on the file, which fails while it is open (object in use), and ends the
; program.
; Correct: after the error the file is closed - the exclusive lock works.
DEFTYPE .l

!TestCase{after_error}
  SetErr
    l=Lock_("CORPUS:novhdr.8svx",-1)
    got=0
    If l Then got=1 : UnLock_ l
    !CheckEq{got,1,"exclusive_lock"}
    End
  End SetErr
  LoadSound 0,"CORPUS:novhdr.8svx"
  !Check{0,"not_refused"}
!EndTestCase
