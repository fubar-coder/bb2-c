; expr-bit-byte-stale (#1217) - a Byte AND, OR or EOR (also & and |) in
; the right operand of a comparison whose left operand is a Word or Long, or
; that is converted to one by the destination.  Its result has a stale upper
; byte when the operands differ in sign (-2 AND 0 is $FF00, 3 OR -2 is
; $00FF; #1218), and the comparison widened the Byte operation without
; clearing it: the port gave 0 for r=(0=(-2 AND 0)) since #1177, the original
; -1.
; Correct: the Byte operation is a Byte (-2 AND 0 is 0, 3 OR -2 is -1), so
; each comparison gives what the values give (-1 true, 0 false).  The
; original and the port agree on every check here; the forms of the
; original's own defect, a Byte operation as the left operand
; (compiler-byte-bitop-stale, #1218), are not here.
Function.b fbm{}
  Function Return -2
End Function
Function.l fl{x.l}
  Function Return x
End Function
!TestCase{bit_compare}
  mb.b=-2 : zb.b=0 : tb.b=3 : ob.b=1 : fb.b=-4
  z.l=0 : tl.l=3 : zw.w=0 : q.q=0 : m2.l=-2 : mw.w=-2
  r.l=0 : rw.w=0 : rq.q=0 : rb.b=0
  r=(0=(-2 AND 0))
  !CheckEq{r,-1,"and_constants"}
  r=(3<(3 OR -2))
  !CheckEq{r,0,"or_constants"}
  r=(0=(-1 EOR -1))
  !CheckEq{r,-1,"eor_equal"}
  r=(-2=(1 EOR -1))
  !CheckEq{r,-1,"eor_constants"}
  r=(0=(-2 & 0))
  !CheckEq{r,-1,"ampersand"}
  r=(3<(3 | -2))
  !CheckEq{r,0,"bar"}
  r=(-1=(NOT 0))
  !CheckEq{r,-1,"not"}
  r=(0=((-2 AND 0) LSL 1))
  !CheckEq{r,-1,"nested_shift"}
  r=(-2=(-4 ASR 1))
  !CheckEq{r,-1,"shift_clean"}
  r=(z=(mb AND zb))
  !CheckEq{r,-1,"long_variables"}
  r=(tl<(tb OR mb))
  !CheckEq{r,0,"long_or_variables"}
  rw=(zw=(mb AND zb))
  !CheckEq{rw,-1,"word_destination"}
  r=0 : If zw=(mb AND zb) Then r=-1
  !CheckEq{r,-1,"if_word"}
  r=0 : If z=(mb AND zb) Then r=-1
  !CheckEq{r,-1,"if_long"}
  r=0 : If 0=(-2 AND 0) Then r=-1
  !CheckEq{r,-1,"if_constant"}
  r=0 : If zw=(-2 AND 0) Then r=-1
  !CheckEq{r,-1,"if_word_constant"}
  r=0 : If z=(-2 AND 0) Then r=-1
  !CheckEq{r,-1,"if_long_constant"}
  r=(m2=(ob EOR -1))
  !CheckEq{r,-1,"eor_variable"}
  r=((-2 AND 0)=0)
  !CheckEq{r,-1,"left_constants"}
  rq=(0=(-2 AND 0))
  !CheckEq{rq,-1,"quick_destination"}
  r=(q=(mb AND zb))
  !CheckEq{r,-1,"quick_left"}
  rb=(0=(-2 AND 0))
  !CheckEq{rb,-1,"byte_destination"}
  r=(1=(3 AND 1))
  !CheckEq{r,-1,"clean_and"}
  r=(-2>(1 OR -4))
  !CheckEq{r,-1,"or_greater"}
  r=(-1=(-1 AND -1))
  !CheckEq{r,-1,"and_minus_one"}
  r=(0=(mb AND 0))
  !CheckEq{r,-1,"and_zero"}
  r=(z=(m2 AND z))
  !CheckEq{r,-1,"long_operands"}
  r=(0=(-2 AND zb))
  !CheckEq{r,-1,"constant_variable"}
  r=(-3=(NOT 2))
  !CheckEq{r,-1,"not_two"}
  r=(2=(-(-2)))
  !CheckEq{r,-1,"unary_minus"}
  r=(0=(-300 AND 0))
  !CheckEq{r,-1,"word_constant_operand"}
  r=(0=(-2 AND 0)+0)
  !CheckEq{r,-1,"nested_sum"}
  r=(-256=(-2 AND 0))
  !CheckEq{r,0,"stale_value_not_equal"}
  r=(255=(3 OR -2))
  !CheckEq{r,0,"stale_or_not_equal"}
  r=(m2=fbm{})
  !CheckEq{r,-1,"byte_function"}
  r=(ob=(NOT mb))
  !CheckEq{r,-1,"not_variable"}
  r=(2=(-mb))
  !CheckEq{r,-1,"minus_variable"}
  r=(z=(mb MOD ob))
  !CheckEq{r,-1,"mod"}
  r=(m2=Peek.b(&mb))
  !CheckEq{r,-1,"peek_byte"}
  rq=(q=(mb AND zb))
  !CheckEq{rq,-1,"quick_both"}
  r=(z=(mb & zb))
  !CheckEq{r,-1,"ampersand_variables"}
  r=(z=(mb EOR mb))
  !CheckEq{r,-1,"eor_same"}
  r=(z=(mw AND zw))
  !CheckEq{r,-1,"word_operands"}
  r=fl{0=(-2 AND 0)}
  !CheckEq{r,-1,"long_parameter"}
  r=(0<>(-2 AND 0))
  !CheckEq{r,0,"not_equal"}
  r=(0=(-2 AND 0 AND 0))
  !CheckEq{r,-1,"and_and"}
  r=Val(Str$(mb AND zb))
  !CheckEq{r,0,"str_value"}
  r=0 : If (mb OR tb)<zw Then r=-1
  !CheckEq{r,-1,"if_or_less"}
  r=(-256=(mb AND zb))
  !CheckEq{r,0,"variable_stale_not_equal"}
  zw=0
  r=0 : While zw<>(mb AND zb) : r+1 : zw=-256 : Wend : zw=0
  !CheckEq{r,0,"while"}
!EndTestCase
