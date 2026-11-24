; compiler-quick-negative-const - a negative whole constant under a Quick
; type so far: consttype2 makes a whole number the Quick with that integer
; part and the fraction 0 (Fixed (#1208)).  The original swapped its words
; (.qint: swap d0), which is that only for 0 to 32767: a negative value
; has $FFFF in its upper word, so the swap left the fraction $FFFF and the
; integer part one too high: On (-1) was $FFFFFFFF (-0.0000153) instead
; of $FFFF0000, #c=-3 was $FFFDFFFF (-2.0000153) instead of $FFFD0000.
; A minus sign before a literal or a constant is not affected: the
; positive value is made a Quick and then negated.
; Correct: q=On is -1, q=#c is -3, 1+On is 0, the other operands of a
; Quick expression see the right value, -32768 is $80000000, 32767 and a
; positive constant are unchanged, and -3 and -#d stay -3.
#c=-3
#d=3
#e=-32768
#g=32767
!TestCase{negative}
  q.q=On
  !CheckQEq{q,-1,"on"}
  q=#c
  !CheckQEq{q,-3,"constant"}
  q=1+On
  !CheckQEq{q,0,"sum"}
  q=0.5+#c
  !CheckQEq{q,-2.5,"half"}
  q=#c*0.5
  !CheckQEq{q,-1.5,"product"}
  q=#e
  !CheckQEq{q,-32768,"lowest"}
  q=-3
  !CheckQEq{q,-3,"literal"}
  q=-#d
  !CheckQEq{q,-3,"negated"}
  q=#g
  !CheckQEq{q,32767,"highest"}
  q=0.25+#d
  !CheckQEq{q,3.25,"positive"}
!EndTestCase
; The flag toggles of EXAMPLES/ANDREWSDEMOS/DUAL.BB (a default-typed high
; set to On or Off, then high=NOT high none, one or two times, then
; If high) keep the original's truth values: NOT of the Quick -1.0 is 0
; (#1209: Not of the whole part).
!TestCase{flags}
  high.q=On : high=NOT high
  !CheckQEq{high,0,"on_not"}
  f.q=0 : high=On : If high Then f=1
  !CheckQEq{f,1,"on"}
  f=0 : high=On : high=NOT high : If high Then f=1
  !CheckQEq{f,0,"on_once"}
  f=0 : high=On : high=NOT high : high=NOT high : If high Then f=1
  !CheckQEq{f,1,"on_twice"}
  f=0 : high=Off : If high Then f=1
  !CheckQEq{f,0,"off"}
  f=0 : high=Off : high=NOT high : If high Then f=1
  !CheckQEq{f,1,"off_once"}
  f=0 : high=Off : high=NOT high : high=NOT high : If high Then f=1
  !CheckQEq{f,0,"off_twice"}
!EndTestCase
