; modlib-word-min-mod - a Word Mod of -32768 by -1 (modlib's domodword):
; the quotient 32768 does not fit DIVS.W's word, which leaves d0 as the
; dividend extended ($FFFF8000), and the low word after the SWAP, -1, is
; taken as the remainder: the original gives -2 (the divisor added to it),
; the port -1 (the floor rule of #1197 adds nothing for the same sign).
; Correct: the remainder 0, as for every other divisor that divides -32768.
!TestCase{word_min}
  w1.w=-32768 : w2.w=-1 : rw.w=5
  rw=w1 Mod w2
  !CheckEq{rw,0,"remainder"}
  w2=2
  rw=w1 Mod w2
  !CheckEq{rw,0,"divisor_2"}
!EndTestCase
