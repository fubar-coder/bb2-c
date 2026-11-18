; ridatatypeslib-create-without-file (#601) - DataTypeCreate &taglist
; needs an open file, DataTypeCreate without a tag list does not: it
; makes an object of the name the last DataTypeOpen copied (at first "",
; then also the name of a file closed since).  The program calls it
; before any DataTypeOpen and after the picture (generated) is closed
; again.
; Correct: without an open file DataTypeCreate returns 0.
; @fixed #601
; @rwe ridatatypeslib
; @pass file=p.ilbm|@TESTDATA@/ridatatypeslib-create-without-file.ilbm
DEFTYPE .l
NPrint "before open ",DataTypeCreate<>0
DataTypeClose
If DataTypeOpen("CORPUS:p.ilbm")
  NPrint "open ",DataTypeCreate<>0
  DataTypeClose
EndIf
NPrint "after close ",DataTypeCreate<>0
DataTypeClose
End
