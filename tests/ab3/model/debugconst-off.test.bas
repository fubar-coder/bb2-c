; XTRA
; Embedded .xtra Header
;
; Debugger
; -------------------------------------------------------
; RuntimeDebug    = 0
; /XTRA
; debugconst-off - #__debug without runtime error checking: 0; a Stop
; inside CNIF #__debug is skipped; a source's own #__debug replaces the
; predefined one
!TestCase{debugconst_off}
  !CheckEq{#__debug,0,"l1"}
  zs$=""
  CNIF #__debug : Stop : CEND
  CNIF #__debug = 0
  zs$="release build"
  CEND
  !CheckStrEq{zs$,"release build","l2"}
  #__debug = 5
  !CheckEq{#__debug,5,"l3"}
  !CheckStrEq{"end","end","l4"}
!EndTestCase
