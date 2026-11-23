; ridatatypeslib-modeid-writes-taglist (#603) - DataTypeModeID taglist
; writes its own tags (BIDTAG_NominalWidth, NominalHeight, Depth and
; TAG_DONE: seven longs) over the start of the program's tag list and
; passes that to BestModeIDA.  The list here is TAG_IGNORE (1) with a
; marker, TAG_DONE, then five more markers; the picture is generated.
; Correct: the program's list is not changed (and the mode is the one
; DataTypeModeID gives without a list: the list adds nothing).
DEFTYPE .l

!TestCase{tag_list}
  Dim t.l(7)
  t(0)=1 : t(1)=$11111111 : t(2)=0
  For i=3 To 7 : t(i)=$22222200+i : Next
  opened=DataTypeOpen("CORPUS:p.ilbm")
  !Require{opened<>0,"opened"}
  o=DataTypeCreate
  b=DataTypeBitmap
  !Require{b<>0,"bitmap"}
  m=DataTypeModeID
  n=DataTypeModeID(&t(0))
  same=(m=n)
  !CheckEq{same,-1,"same_mode"}
  k=-1
  If t(0)<>1 OR t(1)<>$11111111 OR t(2)<>0 Then k=0
  For i=3 To 7
    If t(i)<>$22222200+i Then k=0
  Next
  !CheckEq{k,-1,"list_kept"}
  DataTypeClose
!EndTestCase
