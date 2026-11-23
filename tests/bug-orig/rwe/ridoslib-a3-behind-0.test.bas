; ridoslib-a3-behind-0 (#618) - GetPathName and NextDevice copy their
; result with the terminating 0 and leave a3 behind the 0 instead of on
; it.  In a string expression the next part is then written one byte
; too far: the result has the right length but holds the 0 behind the
; result, and the last character of the next part is lost.
; Correct: GetPathName("DH0:dir/file")+"x" is "DH0:dir/x" (length 9, the
; 9th character "x" = 120); NextDevice+"x" (the first volume) ends in
; "x" and holds no 0.
DEFTYPE .l

!TestCase{string_expressions}
  a$=GetPathName("DH0:dir/file")+"x"
  !CheckEq{Len(a$),9,"path_length"}
  !CheckEq{Asc(Mid$(a$,8)),47,"path_slash"}
  !CheckEq{Asc(Mid$(a$,9)),120,"path_last"}
  r=GetDeviceList(8)
  !Require{r<>0,"list"}
  b$=NextDevice+"x"
  FreeDeviceList
  z=0
  For i=1 To Len(b$)
    If Asc(Mid$(b$,i,1))=0 Then z=z+1
  Next
  !CheckEq{Asc(Right$(b$,1)),120,"volume_last"}
  !CheckEq{z,0,"volume_zeros"}
!EndTestCase
