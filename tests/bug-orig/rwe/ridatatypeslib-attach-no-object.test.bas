; ridatatypeslib-attach-no-object (#1098) - AttachDataType in the forms
; window,id,x,y,width,height and window,id,&taglist checks the open file
; but not the object: with a file open and no DataTypeCreate both pass
; the object 0 to SetAttrsA and AddDTObject and report it as attached.
; The picture (generated) is opened, no object is created; each form is
; followed by DetachDataType, so the second is not refused as attached
; already.
; Correct: both forms return 0 without an object.
DEFTYPE .l

!TestCase{no_object}
  Dim t.l(2)
  t(0)=$80030001 : t(1)=5 : t(2)=0
  WbToScreen 0
  Window 0,0,0,320,100,$100F,"DT",1,2
  opened=DataTypeOpen("CORPUS:p.ilbm")
  !Require{opened<>0,"opened"}
  r=AttachDataType(0,1,2,3,16,2)
  coords=(r<>0)
  !CheckEq{coords,0,"coordinates"}
  DetachDataType
  r=AttachDataType(0,1,&t(0))
  tags=(r<>0)
  !CheckEq{tags,0,"taglist"}
  DetachDataType
  DataTypeClose
  Free Window 0
!EndTestCase
