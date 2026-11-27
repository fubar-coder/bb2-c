; XTRA
; Embedded .xtra Header
;
; Debugger
; -------------------------------------------------------
; RuntimeDebug    = 1
; /XTRA
; debugconst-on - #__debug with runtime error checking (the header's
; RuntimeDebug): -1, in an expression, CNIF and a constant definition;
; with -r (runtime-errors = "off") 0 (debugconst-on.test.toml)
!TestCase{debugconst_on}
  !CheckEq{#__debug,-1,"l1"}
  zs$=""
  CNIF #__debug
  zs$=zs$+"debug build"
  CEND
  CNIF #__debug = 0
  zs$=zs$+"release build"
  CEND
  !CheckStrEq{zs$,"debug build","l2"}
  #d = #__debug
  !CheckEq{#d,-1,"l3"}
!EndTestCase
