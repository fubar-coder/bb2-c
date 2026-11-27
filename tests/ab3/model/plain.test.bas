; plain - the run-output cases of AB3 mode without a compiler state of
; their own (the default OPTIMIZE and SYNTAX, no header), in one file so
; that AmiBlitz3 compiles them in one session. Each case was a case of
; tests/ab3/CMakeLists.txt and has its name.
!TestCase{plus}
  ; the + prefix of a statement (no type check): compiled as without it
  b.w = 3
+a.l = b.w
  !CheckEq{a,3,"l1"}
!EndTestCase
