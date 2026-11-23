; rimapeditorlib-mapobjname-byte-length (#899) - MapObjName$ copies all
; characters of the map's name but counts them with ADDQ.b: the length
; of the result is the count modulo 256.  A name of 300 characters comes
; back with the length 44.
; Correct: the name comes back whole, 300 characters.
DEFTYPE .l

!TestCase{name_300}
  CreateMap 0,4,4,16,16,"m"
  SetMapObjName$ String$("a",300)
  a$=MapObjName$
  nl=Len(a$)
  w=0
  If a$=String$("a",300) Then w=1
  !CheckEq{nl,300,"length"}
  !CheckEq{w,1,"name_whole"}
!EndTestCase
