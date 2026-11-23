; freqlib-lengths (#153) - on OK FileRequest$ copies the path and the file
; name into Pathname$ and Filename$ but does not set their lengths: they
; keep the lengths they had before.  The program runs through inject in
; CORPUS:a (cd=; dirs makes it) with an empty path, so PARENT makes the
; path "CORPUS:" (the parent of the current directory); the file name is
; "x" followed by a 0 and "yz" (length 4: the requester takes "x").
; inject clicks PARENT, then OK (click=).
; Own 320x256 screen (see t93_freq).
; Correct: the lengths are those of the new contents: Pathname$ the root
; of the volume (the name of the cell's volume, "VOL:"), Filename$ "x"
; (1).  The original leaves them at 0 and 4.
DEFTYPE .l

!TestCase{lengths}
  Screen 0,0,0,320,256,2,0,"freqlib",1,2
  Use Screen 0
  MaxLen p$=160
  MaxLen f$=64
  p$=""
  f$="x"+Chr$(0)+"yz"
  r$=FileRequest$("FQ",p$,f$)
  !CheckStrEq{Right$(r$,2),":x","result_tail"}
  !Check{Len(p$)>1 AND Instr(p$,":")=Len(p$),"path_is_root"}
  !CheckEq{Len(f$),1,"file_len"}
!EndTestCase
