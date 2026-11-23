; ridatatypeslib-attach-words-as-longs (#597) - AttachDataType
; window,id,x,y,width,height declares x, y, width and height as words
; but writes the registers into its tag list as longs, with whatever
; upper words they have: a negative word is not sign-extended, and an
; upper word left by the code before the call goes into the value.  The
; program passes word variables (x -5, y 3) and constants (16, 2) and
; reads the tag list the library gave SetAttrs: the library's buffer,
; whose address GetDataTypeAttr returns (GA_Left's value at +12,
; GA_Top's at +20, GA_Width's at +28, GA_Height's at +36).  The picture
; is generated.  The upper words depend on the memory layout (they
; are addresses the code before left), so the program prints only
; whether all four values are right.
; Correct: the tags hold the arguments as longs: -5, 3, 16, 2.
DEFTYPE .l

!TestCase{tags}
  x.w=-5 : y.w=3
  WbToScreen 0
  Window 0,0,0,320,100,$100F,"DT",1,2
  opened=DataTypeOpen("CORPUS:p.ilbm")
  !Require{opened<>0,"opened"}
  o=DataTypeCreate
  !Require{o<>0,"object"}
  b=GetDataTypeAttr($80001002)
  r=AttachDataType(0,1,x,y,16,2)
  att=(r<>0)
  !CheckEq{att,-1,"attached"}
  k=(Peek.l(b+12)=-5 AND Peek.l(b+20)=3)
  all=(k AND Peek.l(b+28)=16 AND Peek.l(b+36)=2)
  !CheckEq{all,-1,"tags_as_longs"}
  DetachDataType
  DataTypeClose
  Free Window 0
!EndTestCase
