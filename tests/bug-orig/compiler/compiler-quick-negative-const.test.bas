; compiler-quick-negative-const - a negative whole constant under a Quick
; type so far: consttype2 makes a whole number a Quick by swapping its
; words (.qint: swap d0), which is right for 0 to 32767 only.  A
; negative value has $FFFF in its upper word, so the swap leaves the
; fraction $FFFF and the integer part one too high: On (-1) is $FFFFFFFF
; (-0.0000153) instead of $FFFF0000, #c=-3 is $FFFDFFFF (-2.0000153)
; instead of $FFFD0000.  A minus sign before a literal or a constant is
; not affected: the positive value is made a Quick and then negated.
; Correct: q=On is -1, q=#c is -3, 1+On is 0, and -3 and -#d stay -3.
#c=-3
#d=3
!TestCase{negative}
  q.q=On
  !CheckQEq{q,-1,"on"}
  q=#c
  !CheckQEq{q,-3,"constant"}
  q=1+On
  !CheckQEq{q,0,"sum"}
  q=-3
  !CheckQEq{q,-3,"literal"}
  q=-#d
  !CheckQEq{q,-3,"negated"}
!EndTestCase
