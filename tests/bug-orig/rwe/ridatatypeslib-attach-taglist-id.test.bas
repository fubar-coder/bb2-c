; ridatatypeslib-attach-taglist-id (#596) - AttachDataType
; window,id,&taglist passes the id as the tag list of its second
; SetAttrsA and never uses the program's tag list.  The id here is 0 (an
; empty tag list in the original, so no other memory is read as tags);
; the tag list sets GA_Left ($80030001) to 5 and GA_Top ($80030003) to
; 3.  The program reads the position of the attached object (a gadget:
; LeftEdge at +4, TopEdge at +6) of the picture (generated).
; Correct: the object is at the position of the tag list (5, 3).
DEFTYPE .l

!TestCase{position}
  Dim t.l(4)
  t(0)=$80030001 : t(1)=5 : t(2)=$80030003 : t(3)=3 : t(4)=0
  WbToScreen 0
  Window 0,0,0,320,100,$100F,"DT",1,2
  opened=DataTypeOpen("CORPUS:p.ilbm")
  !Require{opened<>0,"opened"}
  o=DataTypeCreate
  !Require{o<>0,"object"}
  r=AttachDataType(0,0,&t(0))
  att=(r<>0)
  !CheckEq{att,-1,"attached"}
  !CheckEq{Peek.w(o+4),5,"left"}
  !CheckEq{Peek.w(o+6),3,"top"}
  DetachDataType
  DataTypeClose
  Free Window 0
!EndTestCase
