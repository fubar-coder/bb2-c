; compiler-macro-last-token-colon (#1206) - domacro stores a ':' for every
; line end of the macro text, and the clean-up .mclean removes the trailing
; ones, but keeps a ':' when the byte in front of it has bit 7 set (meant
; for a token whose low byte is $3A, the code of ':').  That byte is also
; the low byte of a whole token whose number has bit 7 set (NOT $8089, OR,
; MOD, Len, NPrint, ...): a macro whose last line ends in such a token
; keeps the ':' of its line end, and a calling line that goes on after the
; macro is split there (c = !mnot b becomes c = NOT: b, "Syntax Error").
; The boundaries: a macro ending in a token whose low byte is $3A
; (ParentOfFH_, $FE3A) keeps the whole token, and a call that ends its
; statement (f = !mpi, the body Pi: f = Pi:) was never affected.
; Correct: every macro expands to its text alone, the calling line goes on.
DEFTYPE .l
Macro mnot
NOT
End Macro
Macro mor
b OR
End Macro
Macro mmod
b MOD
End Macro
Macro mlen
Len
End Macro
Macro mnprint
NPrint
End Macro
Macro mparent
ParentOfFH_
End Macro
Macro mpi
Pi
End Macro
Macro mnotb
NOT b
End Macro

!TestCase{not_end}
  b = 3
  c = !mnot b
  !CheckEq{c,-4,"not"}
!EndTestCase

!TestCase{or_end}
  b = 3
  c = !mor 4
  !CheckEq{c,7,"or"}
!EndTestCase

!TestCase{mod_end}
  b = 7
  c = !mmod 4
  !CheckEq{c,3,"mod"}
!EndTestCase

!TestCase{len_end}
  c = !mlen("abc")
  !CheckEq{c,3,"len"}
!EndTestCase

!TestCase{nprint_end}
  c = 1 : !mnprint "x" : c = 2
  !CheckEq{c,2,"after_nprint"}
!EndTestCase

!TestCase{token_low_3a}
  ; the macro ends in ParentOfFH_ ($FE3A): its low byte stays
  fh = Open_("RAM:compiler-macro-last-token-colon.tmp",1006)
  !Require{fh<>0,"open"}
  lk = !mparent(fh)
  Close_ fh
  !Check{lk<>0,"parent_lock"}
  If lk Then UnLock_ lk
  n$ = "RAM:compiler-macro-last-token-colon.tmp"
  DeleteFile_ &n$
!EndTestCase

!TestCase{statement_end}
  b = 3
  c = !mnotb
  !CheckEq{c,-4,"not_b"}
  f.f = !mpi
  !CheckFltNear{f,3.14159,0.001,"pi"}
!EndTestCase
