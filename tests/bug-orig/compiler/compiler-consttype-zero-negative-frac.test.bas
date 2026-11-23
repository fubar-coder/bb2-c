; compiler-consttype-zero-negative-frac (#13) - consttype2 builds a Quick
; constant from the integer part and the fraction * 65536, and takes a
; fraction of 0 (beq) or a negative one (unsigned bcc) for one that does
; not fit: "q=1.0" compiles a float constant and its conversion to Quick
; where "q=1.5" is a Quick constant. (A literal has no sign, so a negative
; fraction does not come from a source.)
; Here the two assignments stand between labels (no check inside: its
; code would have a length of its own) and the lengths of their code are
; compared.
; Correct: 1.0 is the Quick constant 1, the code of q=1.0 is as long as
; that of q=1.5.
!TestCase{zero_fraction}
.l1
  q.q=1.0
.l2
  q=1.5
.l3
  n1.l=?l2-?l1
  n2.l=?l3-?l2
  !CheckQEq{q,1.5,"half"}
  q=1.0
  !CheckQEq{q,1,"one"}
  !CheckEq{n1,n2,"same_code_length"}
!EndTestCase
