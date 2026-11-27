; XTRA
; Embedded .xtra Header
;
; General Info
; -------------------------------------------------------
; Residents       = ""
; /XTRA
; pow - the float power in AB3 mode: AmiBlitz3's math library (44), not
; BB2's 192. The header above is the compiler's input (no resident).
!TestCase{pow}
  a.f = 2
  !CheckFltNear{a^10,1024,0.001,"l1"}
  b.f = 1.5
  ; the old expect line was the pattern ~2.2#: 2.2 and more digits
  !CheckFltNear{b^2,2.25,0.05,"l2"}
!EndTestCase
