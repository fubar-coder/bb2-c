; trackerlib-loadmodule-leak (#296) - LoadModule gets a FileInfoBlock
; (260 bytes, memlib globalloc) and a lock on the name; when Examine fails
; or the name is a drawer it goes into the runtime error "Error Accessing
; Module File" without freeing the FileInfoBlock and without UnLock: a
; program that catches the error (SetErr) and goes on has lost the
; memory, and the drawer stays locked.  The program makes a drawer in
; RAM: and loads it as a module.
; After the error the program asks for an exclusive lock on the drawer,
; which fails while the shared lock of LoadModule is open (object in
; use).
; Correct: after the error the memory is as before LoadModule, and the
; exclusive lock works (no lock is left on the drawer).
; @fixed #296
; @configs clib
DEFTYPE .l
k=CreateDir_("RAM:t296")
If k=0 Then NPrint "no drawer" : End
UnLock_ k
NPrint "start"
SetErr
  a=AvailMem_(1)
  NPrint "error"
  If b-a>=260
    NPrint "FileInfoBlock kept"
  Else
    NPrint "FileInfoBlock freed"
  EndIf
  l=Lock_("RAM:t296",-1)
  NPrint "exclusive lock ",l<>0
  If l Then UnLock_ l
  End
End SetErr
b=AvailMem_(1)
LoadModule 0,"RAM:t296"
NPrint "loaded"
End
