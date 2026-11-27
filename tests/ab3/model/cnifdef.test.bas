; cnifdef - AB3 mode: CNIF @ - every #name of the condition is a "defined?"
; test giving 1 or 0, without an error for an undefined one; #__debug is 0
; (-r: cnifdef.test.toml; AmiBlitz3's command line has no debugger: 0
; there)
#zq = 5
!TestCase{cnifdef}
  zs$=""
  CNIF @#zq = 1
  zs$="zq defined"
  CEND
  !CheckStrEq{zs$,"zq defined","l1"}
  zs$=""
  CNIF @#zr = 0
  zs$="zr undefined"
  CEND
  !CheckStrEq{zs$,"zr undefined","l2"}
  zs$=""
  CNIF @#zq + #zq = 2
  zs$="both are tests"
  CEND
  !CheckStrEq{zs$,"both are tests","l3"}
  zs$=""
  CNIF @#zq + #zr = 1
  zs$="no error"
  CEND
  !CheckStrEq{zs$,"no error","l4"}
  zs$=""
  CNIF @#__include = 1
  zs$="__include defined"
  CEND
  !CheckStrEq{zs$,"__include defined","l5"}
  zs$=""
  CNIF #__debug = 0
  zs$="no debugger"
  CEND
  !CheckStrEq{zs$,"no debugger","l6"}
  zs$=Str$(#__include)+" "+Str$(#__debug)+" "+Str$(#zq)
  !CheckStrEq{zs$,"0 0 5","l7"}
!EndTestCase
