; ridatatypeslib-close-not-detached (#599) - DataTypeClose disposes of the
; object without removing it from the window AttachDataType added it to,
; and leaves `attached` set: the window keeps the freed gadget, and a
; later AttachDataType returns 0 without attaching.  The picture
; (generated) is attached, closed, opened again and attached again.
; Correct: DataTypeClose detaches the object - the second AttachDataType
; succeeds.
DEFTYPE .l

!TestCase{two_attaches}
  WbToScreen 0
  Window 0,0,0,320,100,$100F,"DT",1,2
  opened=DataTypeOpen("CORPUS:p.ilbm")
  !Require{opened<>0,"opened"}
  o=DataTypeCreate
  b=DataTypeBitmap
  r=AttachDataType(0,1)
  first=(r<>0)
  !CheckEq{first,-1,"first_attach"}
  DataTypeClose
  opened=DataTypeOpen("CORPUS:p.ilbm")
  !Require{opened<>0,"opened_again"}
  o=DataTypeCreate
  b=DataTypeBitmap
  r=AttachDataType(0,1)
  second=(r<>0)
  !CheckEq{second,-1,"second_attach"}
  DetachDataType
  DataTypeClose
  Free Window 0
!EndTestCase
