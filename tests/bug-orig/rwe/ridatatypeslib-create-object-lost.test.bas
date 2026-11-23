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
DEFTYPE .l

!TestCase{two_objects}
  opened=DataTypeOpen("CORPUS:p.ilbm")
  !Require{opened<>0,"opened_warm_up"}
  o=DataTypeCreate
  DataTypeClose
  m0=AvailMem_(0)
  opened=DataTypeOpen("CORPUS:p.ilbm")
  !Require{opened<>0,"opened"}
  o=DataTypeCreate
  first=(o<>0)
  !CheckEq{first,-1,"first"}
  o=DataTypeCreate
  second=(o<>0)
  !CheckEq{second,0,"second"}
  DataTypeClose
  m1=AvailMem_(0)
  lost=(m0-m1>16000)
  !CheckEq{lost,0,"lost"}
!EndTestCase
