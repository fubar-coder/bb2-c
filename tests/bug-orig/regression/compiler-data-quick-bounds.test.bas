; compiler-data-quick-bounds (#1074) - the Data.q values next to -32768,
; which the original already reads right: Data reads the '-' itself, and
; -32767, -1, 0 and 32767 fit a Quick as integers; -32768 in an
; expression is the long 32768, negated and converted afterwards, also
; when the expression goes on computing with the long.
; Correct: $80010000, $FFFF0000, 0, $7FFF0000; q=-32768 is $80000000,
; q=-32768*4/8 is -16384 ($C0000000).
; A regression test of the fix of #1074: the original already gives this
; (correct) result.

.qdata
Data.q -32767,-1,0,32767

!TestCase{data_quick_bounds}
  Dim buf.l(1) : m.l=&buf(0)
  Restore qdata
  Read q.q
  Poke.q m,q
  !CheckEq{Peek.l(m),$80010000,"data_minus_32767"}
  Read q
  Poke.q m,q
  !CheckEq{Peek.l(m),$FFFF0000,"data_minus_1"}
  Read q
  Poke.q m,q
  !CheckEq{Peek.l(m),0,"data_zero"}
  Read q
  Poke.q m,q
  !CheckEq{Peek.l(m),$7FFF0000,"data_32767"}
  q=-32768
  Poke.q m,q
  !CheckEq{Peek.l(m),$80000000,"expression_minus_32768"}
  q=-32768*4/8
  Poke.q m,q
  !CheckEq{Peek.l(m),$C0000000,"expression_computing_on"}
!EndTestCase
