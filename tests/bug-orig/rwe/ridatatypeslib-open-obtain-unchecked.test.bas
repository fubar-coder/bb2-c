; ridatatypeslib-open-obtain-unchecked (#605) - DataTypeOpen locks the
; file and does not handle a failing ObtainDataType (it reads the header
; through address 28); DataTypeClose unlocks only when there is a
; DataType: the lock stays, and DataTypeOpen refuses every further file.
; The program opens an empty file and a directory (CORPUS:), for which
; ObtainDataType may fail, closing after each, then the picture
; (generated).
; Correct: the picture opens whatever happened before.
DEFTYPE .l

!TestCase{picture_after}
  r1=DataTypeOpen("CORPUS:e.dat")
  DataTypeClose
  r2=DataTypeOpen("CORPUS:")
  DataTypeClose
  r3=DataTypeOpen("CORPUS:p.ilbm")
  pic=(r3<>0)
  !CheckEq{pic,-1,"picture"}
  DataTypeClose
!EndTestCase
