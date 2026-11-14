; ridoslib-a3-behind-0 (#618) - GetPathName and NextDevice copy their
; result with the terminating 0 and leave a3 behind the 0 instead of on
; it.  In a string expression the next part is then written one byte
; too far: the result has the right length but holds the 0 behind the
; result, and the last character of the next part is lost.
; Correct: GetPathName("DH0:dir/file")+"x" is "DH0:dir/x" (length 9, the
; 9th character "x" = 120); NextDevice+"x" (the first volume) ends in
; "x" and holds no 0.
; @fixed #618
; @rwe ridoslib
DEFTYPE .l
a$=GetPathName("DH0:dir/file")+"x"
NPrint Len(a$)," ",Asc(Mid$(a$,8))," ",Asc(Mid$(a$,9))
If GetDeviceList(8)
  a$=NextDevice+"x"
  FreeDeviceList
  z=0
  For i=1 To Len(a$)
    If Asc(Mid$(a$,i,1))=0 Then z=z+1
  Next
  NPrint "volume: last ",Asc(Right$(a$,1)),", zeros ",z
Else
  NPrint "no list"
EndIf
End
