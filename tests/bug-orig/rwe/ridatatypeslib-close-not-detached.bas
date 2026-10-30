; ridatatypeslib-close-not-detached (#599) - DataTypeClose disposes of the
; object without removing it from the window AttachDataType added it to,
; and leaves `attached` set: the window keeps the freed gadget, and a
; later AttachDataType returns 0 without attaching.  The picture
; (generated) is attached, closed, opened again and attached again.
; Correct: DataTypeClose detaches the object - the second AttachDataType
; succeeds.
; The window takes the output of NPrint: DefaultOutput sends it back.
; @rwe ridatatypeslib
; @pass file=p.ilbm|@TESTDATA@/ridatatypeslib-close-not-detached.ilbm
DEFTYPE .l
WbToScreen 0
Window 0,0,0,320,100,$100F,"DT",1,2
DefaultOutput
If DataTypeOpen("CORPUS:p.ilbm")
  o=DataTypeCreate
  b=DataTypeBitmap
  NPrint "first attach ",AttachDataType(0,1)<>0
  DataTypeClose
EndIf
If DataTypeOpen("CORPUS:p.ilbm")
  o=DataTypeCreate
  b=DataTypeBitmap
  NPrint "second attach ",AttachDataType(0,1)<>0
  DetachDataType
  DataTypeClose
EndIf
End
