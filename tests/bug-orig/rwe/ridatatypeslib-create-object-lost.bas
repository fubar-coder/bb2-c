; ridatatypeslib-create-object-lost (#600) - DataTypeCreate stores the
; new object over the one made before without disposing of it:
; DataTypeClose and finit dispose of the last one only.  The picture
; (generated) is 320 x 200 pixels with four planes (32000 bytes of
; bitmap).  After one open/create/close round (it loads the classes) the
; program compares the free memory before and after a round with two
; DataTypeCreate.
; Correct: the second DataTypeCreate is refused (0) while the first
; object exists (user decision W4 Q4, as DataTypeOpen and AttachDataType
; refuse), and the memory comes back - less than 16000 bytes missing.
; @fixed #600
; @rwe ridatatypeslib
; @pass file=p.ilbm|@TESTDATA@/ridatatypeslib-create-object-lost.ilbm
DEFTYPE .l
If DataTypeOpen("CORPUS:p.ilbm")
  o=DataTypeCreate
  DataTypeClose
EndIf
m0=AvailMem_(0)
If DataTypeOpen("CORPUS:p.ilbm")
  o=DataTypeCreate
  NPrint "first ",o<>0
  o=DataTypeCreate
  NPrint "second ",o<>0
  DataTypeClose
EndIf
m1=AvailMem_(0)
NPrint "lost ",m0-m1>16000
End
