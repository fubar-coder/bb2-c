; fileiolib-eof-loc-lof (#144) - Eof at the end of a file tries a seek one
; byte further outside the buffer; myseek stores the new position as the
; buffer's file position before the dos Seek, and when that Seek fails
; nothing puts it back: Loc and Lof are 1 too high afterwards (and the
; append idiom FileSeek f,Lof(f) asks for a position behind the end).
; Here Eof on a new empty file, then 3 bytes written (Loc/Lof); the file
; read back to its end and Eof (Loc/Lof); Eof after a FileSeek to the end
; of an OpenFile (Loc/Lof).
; Correct: positions count from 0, at the end Loc = Lof = the length, 3.
; The original gives 4.
; @fixed #144
; @configs clib
f$="RAM:fileiolib-eof-loc-lof.dat"
KillFile f$
If WriteFile(0,f$)
  e=Eof(0)
  NPrint "new eof ",e
  FileOutput 0 : Print "abc" : DefaultOutput
  l1.l=Loc(0) : l2.l=Lof(0)
  NPrint "written loc ",l1," lof ",l2
  CloseFile 0
EndIf
NPrint "exists ",Exists(f$)
If ReadFile(1,f$)
  FileInput 1 : a$=Inkey$(3) : DefaultInput
  e=Eof(1)
  l1.l=Loc(1) : l2.l=Lof(1)
  NPrint "read [",a$,"] eof ",e," loc ",l1," lof ",l2
  CloseFile 1
EndIf
If OpenFile(2,f$)
  FileSeek 2,3
  e=Eof(2)
  l1.l=Loc(2) : l2.l=Lof(2)
  NPrint "open seek 3 eof ",e," loc ",l1," lof ",l2
  CloseFile 2
EndIf
NPrint "exists ",Exists(f$)
KillFile f$
End
