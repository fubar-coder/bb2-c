; asm-short-range-back - a short branch (#1075) whose target lies 130
; bytes back, one beyond the -128 of an 8-bit displacement: "Illegal
; Displacement" at the line of the branch (asm-short-range-back.test.toml)
; Correct: the compile ends with the error, nothing runs. (The original
; compiles a word branch; the BRA skips it, which would loop forever.)
!TestCase{range_back}
  BRA skip
  .e1
  Dcb.w 64,$4E71
  BEQ.b e1
  .skip
!EndTestCase
