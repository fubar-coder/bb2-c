; byte-bitop-stale - a Byte AND, OR or EOR of operands of different sign
; (#1218, #1217): the port's byte operation is a Word operation, so no
; stale upper byte (-2 AND 0 is $FF00, 3 OR -2 is $00FF in the original's
; and.b, or.b) is widened as part of the value; every check expects 1
; (true). AmiBlitz3 v3.14.0 keeps the stale byte of the variable forms
; (l1, l3, l5 and l6 give 0 there: byte-bitop-stale.test.toml)
!TestCase{byte_bitop_stale}
  mb.b=-2 : zb.b=0 : tb.b=3 : ob.b=1
  zw.w=0 : mw.w=-1 : z.l=0 : r.l=0
  If (mb AND zb)=zw Then k.l=1 Else k=0
  !CheckEq{k,1,"l1"}
  If (-2 AND 0)=zw Then k=1 Else k=0
  !CheckEq{k,1,"l2"}
  If (mb AND zb)+zw=0 Then k=1 Else k=0
  !CheckEq{k,1,"l3"}
  If ((-2 AND 0) LSL 1)=0 Then k=1 Else k=0
  !CheckEq{k,1,"l4"}
  If (tb OR mb)=mw Then k=1 Else k=0
  !CheckEq{k,1,"l5"}
  If (ob EOR mb)=mw Then k=1 Else k=0
  !CheckEq{k,1,"l6"}
  r=(0=(-2 AND 0))
  !CheckEq{r+2,1,"l7"}
  r=(z=(mb AND zb))
  !CheckEq{r+2,1,"l8"}
  r=(3<(3 OR -2))
  !CheckEq{r+1,1,"l9"}
!EndTestCase
