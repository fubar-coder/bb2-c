; compiler-fixtypes-nofields (#35) - fixtypes finds a type of a resident
; behind its fields (the field list up to next 0, then the length byte at
; +14 of the last field); a NewType without fields has none, so it starts
; on the type entry itself. Here the empty type is the last one (next 0):
; its name byte at +14 ("emp0": '0') is taken as a field length and the
; type is linked 48 bytes too far on, behind the hunk (inject pad: zeros
; there), and the macros and constants behind it are not found either.
; Correct: the resident's constant #c is found, the program reads 7.
; (The resident is compiled before the test program: the file of the
; configuration.)
!TestCase{constant_after_empty_type}
  v.l=#c
  !CheckEq{v,7,"constant"}
!EndTestCase
