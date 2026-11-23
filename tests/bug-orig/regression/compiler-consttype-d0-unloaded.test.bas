; compiler-consttype-d0-unloaded (#12) - a port regression, not a bug of
; the original: for a float literal in a Word expression or in one
; without a type yet, consttype2 jumps to .quick2 without makeint, so d0
; is not the integer part. In the original 1.95 the upper word of d0
; still holds the upper word of the last FFP result (negative), the range
; check fails and the literal is always a float constant. The port passed
; the character after the number with an upper word of 0: "a.w=0.5" at
; the end of the line (character 0) became the Quick fraction $8000
; (-32768), "42.5*" (character '*' = 42) and "43.5+" became Quick
; constants by chance (42.5*1000 overflowed the Quick).
; Correct (what the original does): the float in both cases: a.w=0.5 is
; 0, b.w=1.5 is 1, 42.5*1000 is 42500, and the untyped literals 2.5 and
; 43.5 compile alike (the same code size).

!TestCase{float_literal_d0}
  a.w=0.5
  !CheckEq{a,0,"word_half"}
  b.w=1.5
  !CheckEq{b,1,"word_one_and_half"}
  !CheckStrEq{Str$(42.5*1000),"42500","untyped_times_1000"}
  f.f=1
  l1:s1$=Str$(2.5+f)
  l2:s2$=Str$(43.5+f)
  l3:
  !CheckStrEq{s1$,"3.5","untyped_2_5"}
  !CheckStrEq{s2$,"44.5","untyped_43_5"}
  !Check{?l2-?l1=?l3-?l2,"same_code_size"}
!EndTestCase
