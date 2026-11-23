; freqlib-a3-behind-0 (#161) - FileRequest$ copies its result with the
; terminating 0 and leaves a3 behind the 0 instead of on it.  In a
; string expression the next part is then written one byte too far: the
; result has the right length but holds the 0 behind the name, and the
; last character of the next part is lost.  The program runs through
; inject, which clicks OK (click=); the path is RAM:, the file name x.
; Own 320x256 screen (see t93_freq).
; Correct: FileRequest$(...)+".bak" is "RAM:x.bak" (length 9, the 6th
; character "." = 46, the 9th "k" = 107).
DEFTYPE .l

!TestCase{bak}
  Screen 0,0,0,320,256,2,0,"freqlib",1,2
  Use Screen 0
  MaxLen p$=160
  MaxLen f$=64
  p$="RAM:"
  f$="x"
  r$=FileRequest$("FQ",p$,f$)+".bak"
  !CheckEq{Len(r$),9,"length"}
  !CheckEq{Asc(Mid$(r$,6)),46,"char6"}
  !CheckEq{Asc(Mid$(r$,9)),107,"char9"}
!EndTestCase
