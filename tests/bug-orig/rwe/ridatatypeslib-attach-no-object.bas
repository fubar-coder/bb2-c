; ridatatypeslib-attach-no-object (#1098) - AttachDataType in the forms
; window,id,x,y,width,height and window,id,&taglist checks the open file
; but not the object: with a file open and no DataTypeCreate both pass
; the object 0 to SetAttrsA and AddDTObject and report it as attached.
; The picture (generated) is opened, no object is created; each form is
; followed by DetachDataType, so the second is not refused as attached
; already.
; Correct: both forms return 0 without an object.
; The window takes the output of NPrint: DefaultOutput sends it back.
; @fixed #1098
; @rwe ridatatypeslib
; @pass file=p.ilbm|@TESTDATA@/ridatatypeslib-attach-no-object.ilbm
DEFTYPE .l
Dim t.l(2)
t(0)=$80030001 : t(1)=5 : t(2)=0
WbToScreen 0
Window 0,0,0,320,100,$100F,"DT",1,2
DefaultOutput
If DataTypeOpen("CORPUS:p.ilbm")
  r=AttachDataType(0,1,2,3,16,2)
  NPrint "coordinates ",r<>0
  DetachDataType
  r=AttachDataType(0,1,&t(0))
  NPrint "taglist ",r<>0
  DetachDataType
  DataTypeClose
Else
  NPrint "not opened"
EndIf
End
