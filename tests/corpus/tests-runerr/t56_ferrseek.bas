; t56_ferrseek - fileiolib: FileSeek behind the end of the file (runtime
; error from the normal code; the original jumps into its error check area)
NPrint "open ",WriteFile(0,"RAM:t56.dat")
FileOutput 0 : Print "abc" : DefaultOutput
CloseFile 0
NPrint "read ",ReadFile(0,"RAM:t56.dat")
FileSeek 0,2 : NPrint "loc ",Loc(0)
NPrint "before"
FileSeek 0,10
NPrint "after"
End
