; freqlib-names-limits - the fix of #1042: FileRequest$ takes what fits its
; buffers (159 / 63 characters) and the MaxLen of Pathname$ and Filename$,
; and refuses the rest (freqlib-names-refused: the clicked file names).
; The drawer CORPUS:fqname holds a file of 28 characters; a line shows
; its first 27 characters (320 pixel screen).  inject clicks (click=, one
; window title per call):
;   F1  MaxLen f$=30: the 28-character name, then OK: taken
;   F2  a path of 159 characters ending in "/", PARENT (which adds a "/"),
;       then OK: refused, the path keeps its 159 characters
;   F3  a start name of 64 characters (MaxLen f$=64), then OK: the name is
;       not taken, the file field is empty
; Without the fix F2 writes the 0 behind the path buffer (into the FILE:
; gadget) and F3 copies without end at OK (a crash).

DEFTYPE .l

!TestCase{names_within_limits}
  Screen 0,0,0,320,256,2,0,"freqlib",1,2
  Use Screen 0
  !Require{WriteFile(0,"CORPUS:fqname/m123456789m123456789m1234567"),"file_written"}
  CloseFile 0
  MaxLen p$=160
  MaxLen f$=30
  p$="CORPUS:fqname"
  f$="y"
  r$=FileRequest$("F1",p$,f$)
  !CheckEq{Len(r$),42,"f1_result_length"}
  !CheckEq{Len(f$),28,"f1_file_length"}
  !CheckStrEq{f$,"m123456789m123456789m1234567","f1_file"}
  p$="RAM:"+String$("a",154)+"/"
  f$="x"
  r$=FileRequest$("F2",p$,f$)
  !CheckEq{Len(r$),160,"f2_result_length"}
  !CheckEq{Len(p$),159,"f2_path_length"}
  !CheckStrEq{Right$(p$,3),"aa/","f2_path_end"}
  MaxLen f$=64
  p$="RAM:"
  f$=String$("z",64)
  r$=FileRequest$("F3",p$,f$)
  !CheckEq{Len(r$),4,"f3_result_length"}
  !CheckStrEq{r$,"RAM:","f3_result"}
  !CheckEq{Len(f$),0,"f3_file_length"}
!EndTestCase
