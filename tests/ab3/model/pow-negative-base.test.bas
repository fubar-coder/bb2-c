; XTRA
; Embedded .xtra Header
;
; General Info
; -------------------------------------------------------
; Residents       = ""
; /XTRA
; pow-negative-base - ^ with a negative base (#1189) in AB3 mode:
; AmiBlitz3's math library (44) computes it, as in AmiBlitz3 v3.14.0,
; which drops the sign of the power like the original (BB2 mode has the
; fix in library 192). The case prints the Floats (asserted in
; pow-negative-base.test.toml): Print's form of them is the subject.
!TestCase{pow_negative_base}
  vf.f=-2.5 : vm.w=-2 : nw.w=3 : rl.l=0
  NPrint vf^3
  NPrint vf^nw
  NPrint vm^2
  NPrint vm^-1
  rl=vf^nw : NPrint rl
  NPrint (-3)^3.5
!EndTestCase
