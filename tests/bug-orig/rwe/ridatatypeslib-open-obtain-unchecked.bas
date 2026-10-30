; ridatatypeslib-open-obtain-unchecked (#605) - DataTypeOpen locks the
; file and does not handle a failing ObtainDataType (it reads the header
; through address 28); DataTypeClose unlocks only when there is a
; DataType: the lock stays, and DataTypeOpen refuses every further file.
; The program opens an empty file and a directory (CORPUS:), for which
; ObtainDataType may fail, closing after each, then the picture
; (generated).
; Correct: the picture opens whatever happened before.
; @rwe ridatatypeslib
; @pass file=e.dat|@TESTDATA@/ridatatypeslib-open-obtain-unchecked.dat
; @pass file=p.ilbm|@TESTDATA@/ridatatypeslib-open-obtain-unchecked.ilbm
DEFTYPE .l
NPrint "empty file ",DataTypeOpen("CORPUS:e.dat")<>0
DataTypeClose
NPrint "directory ",DataTypeOpen("CORPUS:")<>0
DataTypeClose
NPrint "picture ",DataTypeOpen("CORPUS:p.ilbm")<>0
DataTypeClose
End
