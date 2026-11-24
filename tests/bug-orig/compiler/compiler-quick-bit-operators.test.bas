; compiler-quick-bit-operators (#1209) - NOT, AND, OR and EOR (also & and
; |) under a Quick type so far.  The original works on the 16.16 bits:
; notit emits not.l, and the Quick column of the operator table has the
; Long handlers, so the fraction bits take part.  NOT inverts them: the
; NOT of a whole number has the fraction $FFFF (h=-1 : h=NOT h gives
; $0000FFFF, 0.99998 and true; x=3 : q=NOT x gives -3.0000153), and AND,
; OR and EOR combine the fractions of both operands (1.5 OR 2.5 is 3.5,
; If h OR g is taken for h=0.5).
; Correct: a Quick operand is converted to a whole number first by
; dropping its 16 fraction bits (rounded down: -1.5 is -2), the result
; stays a Quick: the usual toggles of a default-typed flag give 0, NOT 3
; is -4, NOT 1.5 is -2, NOT -1.5 is 1, 1.5 OR 2.5 is 3, and If h OR g is
; false for h=0.5 and true for h=-0.5.  Whole values with a whole mask
; and the shifts (which work on the 16.16 value) are unchanged.
!TestCase{flags}
  l.l=5
  h.q=-1 : h=NOT h
  !CheckQEq{h,0,"minus1"}
  h=True : h=NOT h
  !CheckQEq{h,0,"true"}
  h=(l=5) : h=NOT h
  !CheckQEq{h,0,"comparison"}
  h=On : h=NOT h
  !CheckQEq{h,0,"on"}
  h=0 : h=NOT h
  !CheckQEq{h,-1,"zero"}
  h=-1 : f.q=0 : If NOT h Then f=1
  !CheckQEq{f,0,"if_minus1"}
  h=0.5 : f=0 : If NOT h Then f=1
  !CheckQEq{f,1,"if_half"}
!EndTestCase
!TestCase{nots}
  x=3 : q.q=1.5
  h.q=NOT x
  !CheckQEq{h,-4,"three"}
  h=NOT 3
  !CheckQEq{h,-4,"constant"}
  h=NOT 1.5
  !CheckQEq{h,-2,"half"}
  h=NOT -1.5
  !CheckQEq{h,1,"negative"}
  h=NOT q
  !CheckQEq{h,-2,"variable"}
  r.l=NOT q : h=r
  !CheckQEq{h,-2,"long"}
!EndTestCase
!TestCase{bits}
  q.q=1.5 : p.q=2.5 : w.w=3 : y=7 : z.q=13
  h.q=q AND p
  !CheckQEq{h,0,"and"}
  h=q OR p
  !CheckQEq{h,3,"or"}
  h=q EOR w
  !CheckQEq{h,2,"eor"}
  h=p & q
  !CheckQEq{h,0,"amp"}
  h=q | p
  !CheckQEq{h,3,"bar"}
  q=-1.5 : h=q AND w
  !CheckQEq{h,2,"and_negative"}
  h=z AND 6
  !CheckQEq{h,4,"mask"}
  h=y OR 8
  !CheckQEq{h,15,"mask_or"}
  h=0.5 : g.q=0 : f.q=0 : If h OR g Then f=1
  !CheckQEq{f,0,"if_half"}
  h=-0.5 : f=0 : If h OR g Then f=1
  !CheckQEq{f,1,"if_negative_half"}
  h=0.5 : g=0.5 : f=0 : If h AND g Then f=1
  !CheckQEq{f,0,"if_half_and"}
!EndTestCase
!TestCase{shifts}
  x=5 : q.q=1.5
  h.q=x LSR 1
  !CheckQEq{h,2.5,"lsr"}
  h=q LSL 1
  !CheckQEq{h,3,"lsl"}
  h=q ASR 1
  !CheckQEq{h,0.75,"asr"}
!EndTestCase
