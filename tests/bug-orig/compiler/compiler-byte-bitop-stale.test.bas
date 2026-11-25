; compiler-byte-bitop-stale (#1218) - a Byte AND, OR or EOR leaves bits 8-15
; of its left operand in the register: a Byte lies there as a sign-extended
; Word, and the byte operation (and.b, or.b, eor.b) changes bits 0-7 only,
; so -2 AND 0 is $FF00 and 3 OR -2 is $00FF.  Converting a Byte up writes
; nothing, so a Word or Long operation next to it takes the stale upper byte
; as part of the value: the bit operation is the left operand (no wanted
; type) of a comparison with a Word, or of a sum, or of a shift.
; Correct: the result is a Byte that widens as itself (-2 AND 0 is 0, 3 OR -2
; is -1, 1 EOR -2 is -1), so every condition here is true.
; The port gave the same wrong results for the right operand of a
; comparison since #1177 (expr-bit-byte-stale, #1217).
!TestCase{bitop_stale}
  mb.b=-2 : zb.b=0 : tb.b=3 : ob.b=1
  zw.w=0 : mw.w=-1 : r.l=0
  r=0 : If (mb AND zb)=zw Then r=-1
  !CheckEq{r,-1,"and_variable"}
  r=0 : If (-2 AND 0)=zw Then r=-1
  !CheckEq{r,-1,"and_constant"}
  r=0 : If (mb AND zb)+zw=0 Then r=-1
  !CheckEq{r,-1,"and_sum"}
  r=0 : If ((-2 AND 0) LSL 1)=0 Then r=-1
  !CheckEq{r,-1,"and_shift"}
  r=0 : If (tb OR mb)=mw Then r=-1
  !CheckEq{r,-1,"or_variable"}
  r=0 : If (3 OR -2)=mw Then r=-1
  !CheckEq{r,-1,"or_constant"}
  r=0 : If (ob EOR mb)=mw Then r=-1
  !CheckEq{r,-1,"eor_variable"}
  r=0 : If (mb & zb)=zw Then r=-1
  !CheckEq{r,-1,"ampersand"}
  r=0 : If (tb | mb)=mw Then r=-1
  !CheckEq{r,-1,"bar"}
  ; right in the original too: Byte against Byte, Word operands
  r=0 : If (mb AND zb)=zb Then r=-1
  !CheckEq{r,-1,"byte_byte"}
  mw=-2 : zw=0
  r=0 : If (mw AND zw)=zw Then r=-1
  !CheckEq{r,-1,"word_word"}
!EndTestCase
