; ridatatypeslib-create-without-file (#601) - DataTypeCreate &taglist
; needs an open file, DataTypeCreate without a tag list does not: it
; makes an object of the name the last DataTypeOpen copied (at first "",
; then also the name of a file closed since).  The program calls it
; before any DataTypeOpen and after the picture (generated) is closed
; again.
; Correct: without an open file DataTypeCreate returns 0.
DEFTYPE .l

!TestCase{create}
  o=DataTypeCreate
  before=(o<>0)
  !CheckEq{before,0,"before_open"}
  DataTypeClose
  opened=DataTypeOpen("CORPUS:p.ilbm")
  !Require{opened<>0,"opened"}
  o=DataTypeCreate
  during=(o<>0)
  !CheckEq{during,-1,"open"}
  DataTypeClose
  o=DataTypeCreate
  after=(o<>0)
  !CheckEq{after,0,"after_close"}
  DataTypeClose
!EndTestCase
