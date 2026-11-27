; XTRA
; Embedded .xtra Header
;
; General Info
; -------------------------------------------------------
; Residents       = "all.res"
;
; Debugger
; -------------------------------------------------------
; RuntimeDebug    = 1
; /XTRA
; debugconst-res - AmiBlitz3's all.res has its own #__debug (0): the
; compile sets it to its value, -1 with runtime error checking (with -r,
; runtime-errors = "off", 0: debugconst-res.test.toml)
!TestCase{debugconst_res}
  !CheckEq{#__debug,-1,"l1"}
!EndTestCase
